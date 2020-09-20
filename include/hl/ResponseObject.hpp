// ResponseObject.hpp
/**\file
 * Response must be a valid json array and finish by '\n' symbol. Also response
 * must has next signature:
 *
 * 1. First value in array must be an number of request message
 *
 * 2. Second value in array must be an json object with next fields(all
 * fields are required):
 *
 *     - version       - version of protocol                        (string)
 *     - id            - client id                                  (string)
 *     - buf_type      - type of buffer which was handle            (string)
 *     - buf_name      - name of buffer which was handle            (string)
 *     - return_code   - in success it is equal to 0                (num)
 *     - error_message - just string with error                     (string)
 *     - tokens        - map where:                                 (map)
 *       - key     - token group                                    (string)
 *       - value   - array of token locations: row, column, len     (arr)
 *
 * \note numeration of lines and columns start from 1
 *
 * \see responseSchema_v1
 * \see responseSchema_v11
 */

#pragma once

#include "RRData.hpp"
#include "hl/Token.hpp"
#include "misc.hpp"
#include <array>
#include <boost/system/error_code.hpp>
#include <list>
#include <string>

namespace nlohmann::json_schema {
class json_validator;
}

namespace hl {
using error_code = boost::system::error_code;
using Validator  = nlohmann::json_schema::json_validator;

struct ResponseObject final {
public:
  int         msg_num;
  std::string version;
  std::string id;
  std::string buf_type;
  std::string buf_name;
  int         return_code;
  std::string error_message;
  TokenList   tokens;
};

class ResponseSerializer {
public:
  ResponseSerializer() noexcept;
  ~ResponseSerializer() noexcept;

  /**\brief serialize the object to string
   *
   * \note if resp is not empty string, then new data will be added in the end
   * of string
   */
  error_code serialize(const ResponseObject &respObj,
                       OUTPUT std::string &resp) noexcept;

private:
  /**\note uses only for debug
   */
  Validator *responseValidator_1_;
  Validator *responseValidator_11_;
};
} // namespace hl
