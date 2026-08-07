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

    bfc::Map<bfc::String, adder::vm::address_t> library;
  };

  template<typename F>
  adder::vm::native_method_t bind(F func) {
    using R    = bfc::function_type<F>::Ret;
    using Args = bfc::function_type<F>::ArgList;

    return [](adder::vm::call_context * ctx) {
      adder::vm::machine * vm = adder::vm::call_context_get_machine(ctx);
      return bfc::invoke(
        func,
        std::make_tuple(*(Args *)adder::vm::call_context_read_arg(ctx, sizeof(Args))...)
      );
    };
  }

  Scripting::Scripting()
    : Subsystem(bfc::TypeID<Scripting>(), "Scripting")
    , m_pBackend(std::make_unique<Backend>()) {
    reload();
  }

  namespace impl {
    adder::vm::address_t symbolLookup(adder::vm::machine * vm, char const * name) {
      Scripting::Backend *pBackend = (Scripting::Backend *) vm->user_data;
      adder::vm::address_t addr     = 0;
      if (!pBackend->library.tryGet(name, &addr))
        return 0;
      return addr;
    }
  }

  bool Scripting::reload() {
    m_pBackend->vm = adder::vm::machine{&m_pBackend->allocator};
    m_pBackend->vm.user_data            = m_pBackend.get();
    m_pBackend->vm.lookup_extern_symbol = impl::symbolLookup;

    bfc::String bootstrapScript;
    bfc::readTextFile("bootstrap.ad", &bootstrapScript);

    std::optional<adder::program> program = adder::compile(bootstrapScript.c_str());
    if (!program.has_value())
      return false;

    auto loaded = adder::vm::load_program(&m_pBackend->vm, program->view());
    auto entrySymbol = program->find_public_symbol("main");
    if (entrySymbol == nullptr)
      return false;

    auto entryHandle = adder::vm::compile_call_handle(&m_pBackend->vm, *entrySymbol);

    adder::vm::call(&m_pBackend->vm, entryHandle);
  }
}

