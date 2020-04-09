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
 *   - id              - client id as number                    (num)
 *   - buf_name        - name of buffer for handling            (string)
 *   - buf_type        - type of buffer entity (f.e. `cpp`)     (string)
 *   - buf_body        - buffer body as plain text              (string)
 *   - additional_info - some information needed for tokenizer  (string)
 */

#include <string>
#include <string_view>

namespace hl {
struct RequestObject final {
public:
  RequestObject() noexcept;

  /**\brief deserialize json request to the object
   *
   * \return in case of success return empty string, otherwise return error
   * string
   */
  std::string deserialize(std::string_view request) noexcept;

  int         msg_num;
  std::string version;
  int         id;
  std::string buf_type;
  std::string buf_name;
  std::string buf_body;
  std::string additional_info;
};
} // namespace hl
