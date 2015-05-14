#include "fetch/cache.hpp"
#include "config.h"

#include <boost/format.hpp>

#ifdef HAVE_SQLITE3
#include <sqlite3.h>
#endif

namespace avecado { namespace fetch {

namespace {

#ifdef HAVE_SQLITE3
namespace sqlite {
struct sqlite_db_deleter {
  void operator()(sqlite3 *ptr) const {
    if (ptr != nullptr) {
      int status = sqlite3_close(ptr);
      if (status != SQLITE_OK) {
        // TODO: use logger
        std::cerr << "Unable to close SQLite3 database\n" << std::flush;
      }
    }
  }
};

struct sqlite_statement_finalizer {
  void operator()(sqlite3_stmt *ptr) const {
    if (ptr != nullptr) {
      int status = sqlite3_finalize(ptr);
      if (status != SQLITE_OK) {
        // TODO: use logger
        std::cerr << "Unable to finalize SQLite3 statement\n" << std::flush;
      }
    }
  }
};

struct statement {
  boost::optional<std::time_t> column_time(int i) {
    if (sqlite3_column_type(ptr.get(), i) == SQLITE_NULL) {
      return boost::none;
    } else {
      sqlite3_int64 t = sqlite3_column_int64(ptr.get(), i);
      return std::time_t(t);
    }
  }

  boost::optional<std::string> column_text(int i) {
    if (sqlite3_column_type(ptr.get(), i) == SQLITE_NULL) {
      return boost::none;
    } else {
      const unsigned char *str = sqlite3_column_text(ptr.get(), i);
      int sz = sqlite3_column_bytes(ptr.get(), i);
      return std::string((const char *)str, sz);
    }
  }

  void column_blob(int i, std::stringstream &stream) {
    const char *bytes = static_cast<const char *>(sqlite3_column_blob(ptr.get(), i));
    int sz = sqlite3_column_bytes(ptr.get(), i);
    stream.write(bytes, sz);
  }

  bool step() {
    int status = sqlite3_step(ptr.get());
    if (status == SQLITE_DONE) { return false; }
    if (status != SQLITE_ROW) {
      throw std::runtime_error((boost::format("Unable to step row in query result: %1%") % sqlite3_errmsg(db_for_errors)).str());
    }
    return true;
  }

  void bind_text(int i, const std::string &str) {
    int sz = str.size();
    char *strp = static_cast<char *>(malloc(sz));
    if (strp == nullptr) { throw std::runtime_error("Unable to allocate memory for string copy."); }
    memcpy(strp, str.c_str(), sz);
    int status = sqlite3_bind_text(ptr.get(), i, strp, sz, &free);
    if (status != SQLITE_OK) {
      free(strp);
      throw std::runtime_error((boost::format("Argument bind failed: %1%") % sqlite3_errmsg(db_for_errors)).str());
    }
  }

  void bind_text(int i, const boost::optional<std::string> &str) {
    if (str) {
      bind_text(i, *str);

    } else {
      int status = sqlite3_bind_null(ptr.get(), i);
      if (status != SQLITE_OK) {
        throw std::runtime_error((boost::format("Argument bind failed: %1%") % sqlite3_errmsg(db_for_errors)).str());
      }
    }
  }

  void bind_time(int i, std::time_t t) {
    int status = sqlite3_bind_int64(ptr.get(), i, sqlite3_int64(t));
    if (status != SQLITE_OK) {
      throw std::runtime_error((boost::format("Argument bind failed: %1%") % sqlite3_errmsg(db_for_errors)).str());
    }
  }

  void bind_time(int i, boost::optional<std::time_t> t) {
    if (t) {
      bind_time(i, *t);

    } else {
      int status = sqlite3_bind_null(ptr.get(), i);
      if (status != SQLITE_OK) {
        throw std::runtime_error((boost::format("Argument bind failed: %1%") % sqlite3_errmsg(db_for_errors)).str());
      }
    }
  }

  void bind_blob(int i, std::stringstream &stream) {
    std::string str = stream.str();
    int sz = str.size();
    char *strp = static_cast<char *>(malloc(sz));
    if (strp == nullptr) { throw std::runtime_error("Unable to allocate memory for blob copy."); }
    memcpy(strp, str.c_str(), sz);
    int status = sqlite3_bind_blob(ptr.get(), i, strp, sz, &free);
    if (status != SQLITE_OK) {
      free(strp);
      throw std::runtime_error((boost::format("Argument bind failed: %1%") % sqlite3_errmsg(db_for_errors)).str());
    }
  }

private:
  friend struct db;
  statement(sqlite3 *db, const std::string &sql) 
    : ptr(), db_for_errors(db) {
    const char *tail = nullptr;
    sqlite3_stmt *ptr_ = nullptr;
    int status = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &ptr_, &tail);
    if (status != SQLITE_OK) {
      throw std::runtime_error((boost::format("Unable to prepare SQLite3 statement \"%1%\": %2%") % sql % sqlite3_errmsg(db_for_errors)).str());
    }
    ptr.reset(ptr_);
  }

  std::unique_ptr<sqlite3_stmt, sqlite_statement_finalizer> ptr;
  sqlite3 *db_for_errors; // use for ERRORS only.
};

struct db {
  db(const std::string &loc) {
    sqlite3 *ptr_;
    int status = sqlite3_open(loc.c_str(), &ptr_);
    if (status != SQLITE_OK) {
      throw std::runtime_error((boost::format("Unable to open SQLite3 database \"%1%\": %2%") % loc % sqlite3_errmsg(ptr_)).str());
    }
    ptr.reset(ptr_);
  }

  statement prepare(const std::string &sql) {
    return statement(ptr.get(), sql);
  }

private:
  std::unique_ptr<sqlite3, sqlite_db_deleter> ptr;
};

} // namespace sqlite

} // anonymous namespace


struct cache::impl {
/** TODO: uncomment me!
  impl(const std::string &loc) 
    : m_db(new sqlite::db(loc)) {

    // ensure that the table for the cache data exists
    sqlite::statement s(m_db->prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='cache'"));
    if (!s.step()) {
      // table doesn't exist, so create it
      m_db->prepare("CREATE TABLE cache (url TEXT PRIMARY KEY, expires INTEGER, last_modified INTEGER, etag TEXT, body BLOB)").step();
    }
  }

  void lookup(std::unique_ptr<request> &req) {
    sqlite::statement s(m_db->prepare("select expires, last_modified, etag, body from cache where url=?"));
    s.bind_text(1, req->url);

    if (s.step()) {
      req->expires = s.column_time(0);
      req->last_modified = s.column_time(1);
      req->etag = s.column_text(2);
      s.column_blob(3, *(req->stream));
    }
  }

  void write(request *req) {
    // first, normalise the request by collapsing any Cache-control / Expires headers.
    if (req->max_age) {
      req->expires = time(nullptr) + *req->max_age;

    } else if (bool(req->expires) && bool(req->base_date)) {
      req->expires = time(nullptr) + (*req->expires - *req->base_date);

    } else {
      req->expires = boost::none;
    }

    sqlite::statement s(m_db->prepare("insert or replace into cache (url, expires, last_modified, etag, body) values (?, ?, ?, ?, ?)"));
    s.bind_text(1, req->url);
    s.bind_time(2, req->expires);
    s.bind_time(3, req->last_modified);
    s.bind_text(4, req->etag);
    s.bind_blob(5, *(req->stream));
    s.step();
  }

private:
  std::unique_ptr<sqlite::db> m_db;
*/
};

#else /* HAVE_SQLITE3 */
struct cache::impl {
  impl(const std::string &) { not_implemented(); }
  void lookup(std::unique_ptr<request> &req) { not_implemented(); }
  void write(request *req) { not_implemented(); }
  void not_implemented() const { 
    throw std::runtime_error("Caching is not implemented because avecado was built without SQLite3 support.");
  }
};
#endif /* HAVE_SQLITE3 */

cache::cache(const std::string &cache_location,
             std::shared_ptr<fetcher> upstream)
  : m_impl() // new impl(cache_location))
  , m_upstream(upstream) {
}

cache::~cache() {
}

std::future<fetch_response> cache::operator()(const request &r) {
  return (*m_upstream)(r);
}

} } // namespace avecado::fetch


