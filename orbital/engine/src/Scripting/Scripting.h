#pragma once

#include "Subsystem.h"
#include "adder/vm.h"

namespace engine {
  class NativeLibrary {
  public:
    struct Method {
      adder::vm::native_method_t proc;
    };

    void* getProcAddress() {
      Method m;
      m.proc = [](adder::vm::call_context * ctx) {
        adder::vm::call_context_read_arg(ctx, 4);
      };
    }

    bfc::Map<bfc::String, Method> methods;
  };

  class Scripting : public Subsystem {
  public:
    struct Backend;

    Scripting();

    bool reload();

    void addNativeLibrary();

  private:
    std::unique_ptr<Backend> m_pBackend;
  };
} // namespace engine