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
 *   - id          - client id as number
 *   - buf_name    - name of buffer for handling
 *   - buf_length  - length of buffer
 *   - buf_body    - buffer body as plain text
 */

#include <string>
#include <string_view>

namespace hl {
struct RequestObject final {
public:
  /**\brief deserialize json request
   *
   * \throw exception if can not deserialize the request
   */
  explicit RequestObject(std::string_view request);

  int         msg_num;
  int         id;
  std::string buf_name;
  std::string buf_body;
};
} // namespace hl
