#include "GLSLPreProcessor.h"
#include "core/Stream.h"

namespace bfc {
  std::optional<String> GLSLPreProcessor::DefaultSrcLoader(URI const & uri) {
    String content;
    if (readTextURI(uri, &content))
      return content;
    else
      return std::nullopt;
  }

  std::optional<URI> GLSLPreProcessor::DefaultSrcFinder(URI const & uri, std::optional<URI> const & basePath) {
    if (!basePath.has_value())
      return uri;
    else
      return basePath->resolveRelativeReference(uri);
  }

  GLSLPreProcessor::GLSLPreProcessor(String const & src, URI const & baseUri, Ref<Context> const & pContext) 
    : m_src(src)
    , m_pContext(pContext)
    , m_baseURI(baseUri)
  {}

  GLSLPreProcessor::GLSLPreProcessor(URI const & resource, Ref<Context> const & pContext)
    : m_baseURI(resource.resolveRelativeReference("../"))
    , m_pContext(pContext)
    , m_src(pContext->loader(resource)) {}

  std::optional<String> bfc::GLSLPreProcessor::process() {
    if (!m_src.has_value())
        return std::nullopt;

    String ret;

    Vector<StringView> lines = m_src->split("\n", SplitFlags_None);
    for (auto& ln : lines) {
      static const StringView prefix = "#include";
      auto trimmed = ln.trim();
      if (!trimmed.startsWith(prefix)) {
        ret = ret + ln + "\n";
        continue;
      }

      StringView includePath = trimmed.substr(prefix.length()).trim(" \r\n\v\"");
      std::optional<URI> resolved = m_pContext->find(includePath, m_baseURI);
      if (!resolved.has_value())
        return std::nullopt;

      ret = ret + "// Begin " + trimmed + "\n";
      if (m_pContext->sources.contains(resolved.value())) {
        ret = ret + m_pContext->sources[resolved.value()] + "\n";
      } else {
        GLSLPreProcessor      srcProcessor(resolved.value(), m_pContext);
        std::optional<String> includeSrc = srcProcessor.process();
        if (!includeSrc.has_value())
          return std::nullopt;

        m_pContext->sources.add(resolved.value(), includeSrc.value());
        ret = ret + includeSrc.value() + "\n";
      }
      ret = ret + "// End " + trimmed + "\n";
    }

    return ret;
  }
} // namespace bfc
