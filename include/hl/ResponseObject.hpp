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
 *     - id            - client id as number from request
 *     - buf_type      - type of buffer which was handle
 *     - buf_name      - name of buffer which was handle
 *     - return_code   - in success it is equal to 0
 *     - error_message - just string with error
 *     - tokens_count  - count of match objects
 *     - tokens        - array of json objects with next signature:
 *       - group - token group
 *       - pos   - array of integes with 3 values: row, column, len
 *
 * \note numeration of lines and columns start from 1
 */

#pragma once

#include "hl/Token.hpp"
#include "misc.hpp"
#include <array>
#include <list>
#include <string>

#define SUCCESS_CODE 0
#define FAILURE_CODE 1

#define NO_ERRORS ""

namespace hl {
struct ResponseObject final {
public:
  /**\brief serialize the object to string
   */
  void dump(OUTPUT std::string &out) const noexcept;

  int         msg_num;
  int         id;
  std::string buf_type;
  std::string buf_name;
  int         return_code;
  std::string error_message;
  TokenList   tokens;
};
} // namespace hl
