#include "Scripting.h"
#include "adder/vm.h"
#include "adder/compiler.h"
#include "adder/program.h"
#include "core/File.h"

namespace engine {
  struct ScriptAssembly {

  };

  struct Scripting::Backend {
    adder::vm::allocator allocator;
    adder::vm::machine   vm = { &allocator };

    bfc::Map<bfc::String, adder::vm::native_method_binding> library;
  };

  int64_t mulBy10(int64_t value)
  {
    return value * 10;
  }

  template<typename R, typename... Args>
  adder::vm::native_method_binding bind(R (*func)(Args...)) {
    using F = R (*)(Args...);

    adder::vm::native_method_binding ret;
    ret.user_data = func;
    ret.callback  = [](adder::vm::call_context * ctx) {
      adder::vm::machine * vm        = adder::vm::call_context_get_machine(ctx);
      void *               user_data = adder::vm::call_context_get_user_data(ctx);

      if constexpr (std::is_same_v<R, void>) {
        bfc::invoke((F)user_data, std::make_tuple(*(Args *)adder::vm::call_context_read_arg(ctx, sizeof(Args))...));
      } else {
        auto args = std::make_tuple(*(Args *)adder::vm::call_context_read_arg(ctx, sizeof(Args))...);
        R * pResult = (R *)adder::vm::call_context_read_arg(ctx, sizeof(R));

        bfc::mem::construct(
          pResult,
          bfc::invoke((F)user_data, std::move(args)));
      }
    };
    return ret;
  }

  Scripting::Scripting()
    : Subsystem(bfc::TypeID<Scripting>(), "Scripting")
    , m_pBackend(std::make_unique<Backend>()) {
    reload();
  }

  Scripting::~Scripting() {
    m_pBackend = nullptr;
  }


  namespace impl {
    adder::vm::native_method_binding symbolLookup(adder::vm::machine * vm, char const * name) {
      Scripting::Backend *pBackend = (Scripting::Backend *) vm->user_data;
      adder::vm::address_t addr     = 0;
      adder::vm::native_method_binding binding;
      if (!pBackend->library.tryGet(name, &binding))
        return {};
      return binding;
    }

    std::string loadModuleSource(adder::vm::machine * vm, char const * name) {
      bfc::String source;
      bfc::Filename path = bfc::String::format("%s.ad", name);
      if (!bfc::readTextFile(path, &source)) {
        return "";
      }
      return source.c_str();
    }
  }

  bool Scripting::reload() {
    m_pBackend->vm = adder::vm::machine{&m_pBackend->allocator};
    m_pBackend->vm.user_data            = m_pBackend.get();
    m_pBackend->vm.lookup_extern_symbol = impl::symbolLookup;
    m_pBackend->vm.load_module_source   = impl::loadModuleSource;

    m_pBackend->library = {
      { "(int64)=>int64:mulBy10", bind(mulBy10) }
    };

    bfc::String bootstrapScript;
    bfc::readTextFile("bootstrap.ad", &bootstrapScript);
    std::optional<adder::program> program = adder::compile(&m_pBackend->vm, "orbital/core", bootstrapScript.c_str());
    if (!program.has_value())
      return false;

    auto loaded = adder::vm::load_program(&m_pBackend->vm, program->view());
    auto entrySymbol = loaded.find_public_symbol("()=>void:main");
    if (entrySymbol == nullptr)
      return false;

    auto resultSymbol = loaded.find_public_symbol("int64:test_result");
    int64_t * pResult = (int64_t *)resultSymbol->data_address;

    auto entryHandle = adder::vm::compile_call_handle(&m_pBackend->vm, *entrySymbol);
    adder::vm::call(&m_pBackend->vm, entryHandle);

    return true;
  }
}

