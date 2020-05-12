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
 *     - id            - client id as number from request           (num)
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
  std::string dump() const noexcept;

  int         msg_num;
  std::string version;
  int         id;
  std::string buf_type;
  std::string buf_name;
  int         return_code;
  std::string error_message;
  TokenList   tokens;
};

const std::string responseSchema_v1 = R"(
{
    "$schema": "http://json-schema/schema#",
    "title": "response schema v1",
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
                    "const": "v1"
                },
                "id": {
                    "comment": "client id",
                    "type": "integer"
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
