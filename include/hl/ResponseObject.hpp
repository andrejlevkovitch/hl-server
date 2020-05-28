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
 *     - id            - client id                             (string, integer)
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
 */

#pragma once

#include "hl/Token.hpp"
#include "misc.hpp"
#include <array>
#include <boost/system/error_code.hpp>
#include <list>
#include <string>
#include <variant>

#define SUCCESS_CODE 0
#define FAILURE_CODE 1

#define NO_ERRORS ""

namespace nlohmann::json_schema {
class json_validator;
}

namespace hl {
using error_code = boost::system::error_code;
using Validator  = nlohmann::json_schema::json_validator;

struct ResponseObject final {
public:
  int                            msg_num;
  std::string                    version;
  std::variant<int, std::string> id;
  std::string                    buf_type;
  std::string                    buf_name;
  int                            return_code;
  std::string                    error_message;
  TokenList                      tokens;
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
  Validator *responseValidator_;
};

const std::string responseSchema_v1 = R"(
{
    "$schema": "http://json-schema/schema#",
    "title": "response schema v1.1",
    "description": "schema for validate response of hl-server",

    "type": "array",
    "items": [
      { "$ref": "#/definitions/message_number" },
      { "$ref": "#/definitions/response_body" }
    ],
    "definitions": {
        "message_number": {
            "type": "integer"
        },
        "response_body": {
            "type": "object",
            "required": [
                "version", "id", "buf_type", "buf_name", "return_code", "error_message", "tokens"
            ],

            "properties": {
                "version": {
                    "comment": "version of protocol",
                    "type": "string",
                    "enum": ["v1", "v1.1"]
                },
                "id": {
                    "comment": "client id",
                    "type": ["string", "integer"]
                },
                "buf_type": {
                    "comment": "type of buffer entity",
                    "type": "string"
                },
                "buf_name": {
                    "comment": "name of buffer",
                    "type": "string"
                },
                "return_code": {
                    "comment": "0 in case of success, otherwise some not null integer value",
                    "type": "integer"
                },
                "error_message": {
                    "comment": "contains inforamtion about error (if some error caused) ",
                    "type": "string"
                },
                "tokens": {
                    "comment": "contains dictionary of tokens by token groups",
                    "$ref": "#/definitions/tokens"
                }
            },
            "additionalProperties": false
        },
        "tokens": {
            "type": "object",
            "patternProperties": {
                "^.+$": {
                    "$ref": "#/definitions/array_of_token_koordinates"
                }
            },
            "additionalProperties": false
        },
        "array_of_token_koordinates": {
            "type": "array",
            "items": {
                "$ref": "#/definitions/token_koordinate"
            }
        },
        "token_koordinate": {
            "comment": "contains array of integers with: row, column, token_size",
            "type": "array",
            "items": {
                "type": "integer"
            },
            "minItems": 3,
            "maxItems": 3
        }
    }
}
)";
} // namespace hl
