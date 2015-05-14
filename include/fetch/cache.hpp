#ifndef FETCHER_CACHE_HPP
#define FETCHER_CACHE_HPP

#include "fetcher.hpp"

#include <memory>
#include <string>

namespace avecado { namespace fetch {

/* Fetcher which caches tiles.
 */
struct cache : public fetcher {
  // caches responses from `upstream` and stores them in
  // `cache_location`.
  cache(const std::string &cache_location, std::shared_ptr<fetcher> upstream);

  virtual ~cache();

  std::future<fetch_response> operator()(const request &);

private:
  struct impl;
  std::unique_ptr<impl> m_impl;
  std::shared_ptr<fetcher> m_upstream;
};

} } // namespace avecado::fetch

#endif /* FETCHER_CACHE_HPP */
