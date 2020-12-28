// RequestObject.hpp
/**\file
 * Request string must contains valid json array and finish by '\n' symbol.
 * Also request must has next signature:
 *
 * 1. First value in array must be an number of message (must be unique)
 *
 * 2. Second value in array must be an json object with next fields (all
 * fields are required):
 *
 *   - version         - version of protocol                    (string)
 *   - id              - client id                              (string)
 *   - buf_type        - type of buffer entity (f.e. `cpp`)     (string)
 *   - buf_name        - name of buffer for handling            (string)
 *   - buf_body        - buffer body as plain text              (string)
 *   - additional_info - some information needed for tokenizer  (string)
 *
 * \see requestSchema_v1
 * \see requestSchema_v11
 */

#include "RRData.hpp"
#include <boost/system/error_code.hpp>
#include <misc.hpp>
#include <string>
#include <string_view>

namespace nlohmann::json_schema {
class json_validator;
}

namespace hl {
using error_code = boost::system::error_code;
using Validator  = nlohmann::json_schema::json_validator;

struct RequestObject final {
public:
  int         msg_num;
  std::string version;
  std::string id;
  std::string buf_type;
  std::string buf_name;
  std::string buf_body;
  std::string additional_info;
};

class RequestDeserializer final {
public:
  RequestDeserializer() noexcept;
  ~RequestDeserializer() noexcept;

  /**\brief deserialize json request to the object
   *
   * \return failure error_code in case of invalid message
   */
  error_code deserialize(std::string_view request,
                         OUTPUT RequestObject &reqObj) noexcept;

private:
  Validator *validator_;
};
} // namespace hl
