#pragma once

#include "core/URI.h"
#include "core/Map.h"

#include <optional>
#include <functional>

namespace bfc {
  class GLSLPreProcessor {
  public:
    static std::optional<String> DefaultSrcLoader(URI const & uri);
    static std::optional<URI>    DefaultSrcFinder(URI const & uri, std::optional<URI> const & basePath);

    struct Context {
      Map<URI, String> sources;

      std::function<std::optional<URI>(URI const &, std::optional<URI> const &)> find   = DefaultSrcFinder;
      std::function<std::optional<String>(URI const &)>                          loader = DefaultSrcLoader;
    };

    GLSLPreProcessor(String const & src, URI const & baseUri, Ref<Context> const & pContext = NewRef<Context>());
    GLSLPreProcessor(URI const & resource, Ref<Context> const & pContext = NewRef<Context>());

    std::optional<String> process();

  private:
    URI                   m_baseURI;
    std::optional<String> m_src;

    Ref<Context> m_pContext;
  };
}
