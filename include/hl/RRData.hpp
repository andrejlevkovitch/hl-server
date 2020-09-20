// RRData.hpp
/**\file
 * Contains Request-Response schemas and others
 */

#pragma once

#include <string>

#define SUCCESS_CODE 0
#define FAILURE_CODE 1

#define NO_ERRORS ""

#define VERSION_V1            "v1"
#define VERSION_V11           "v1.1"
#define BASE_VERSION_PROTOCOL VERSION_V11

#define VERSION_TAG         "version"
#define ID_TAG              "id"
#define BUFFER_TYPE_TAG     "buf_type"
#define BUFFER_NAME_TAG     "buf_name"
#define BUFFER_BODY_TAG     "buf_body"
#define ADDITIONAL_INFO_TAG "additional_info"

// specific response fields
#define RETURN_CODE_TAG   "return_code"
#define ERROR_MESSAGE_TAG "error_message"
#define TOKENS_TAG        "tokens"

namespace hl {
enum class Version { None, V1, V11 };

inline Version getVersionEnum(std::string_view versionString) {
  if (versionString == VERSION_V1) {
    return Version::V1;
  } else if (versionString == VERSION_V11) {
    return Version::V11;
  }

  return Version::None;
}

const std::string baseReqSchema = R"(
{
  "type": "array",
  "items": [
    { "$ref": "#/definitions/message_number" },
    {"$ref": "#/definitions/request_body"}
  ],
  "minItems": 2,
  "maxItems": 2,
  "definitions": {
    "message_number": {
      "type": "integer"
    },
    "request_body": {
      "type": "object",
      "required": [
          "version"
      ],
      "properties": {
        "version": {
          "type": "string",
          "enum": ["v1", "v1.1"]
        }
      }
    }
  }
}
)";

const std::string requestSchema_v1 = R"(
{
    "$schema": "http://json-schema/schema#",
    "title": "request schema v1",
    "description": "schema for validate requests for hl-server",

    "type": "array",
    "items": [
      { "$ref": "#/definitions/message_number" },
      { "$ref": "#/definitions/request_body" }
    ],
    "minItems": 2,
    "maxItems": 2,
    "definitions": {
        "message_number": {
            "type": "integer"
        },
        "request_body": {
            "type": "object",
            "required": [
                "version", "id", "buf_type", "buf_name", "buf_body", "additional_info"
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
                "buf_body": {
                    "comment": "complete buffer entity",
                    "type": "string"
                },
                "additional_info": {
                    "comment": "some handler specific information",
                    "type": "string"
                }
            },
            "additionalProperties": false
        }
    }
}
)";

const std::string requestSchema_v11 = R"(
{
    "$schema": "http://json-schema/schema#",
    "title": "request schema v1.1",
    "description": "schema for validate requests for hl-server",

    "type": "array",
    "items": [
      { "$ref": "#/definitions/message_number" },
      { "$ref": "#/definitions/request_body" }
    ],
    "minItems": 2,
    "maxItems": 2,
    "definitions": {
        "message_number": {
            "type": "integer"
        },
        "request_body": {
            "type": "object",
            "required": [
                "version", "id", "buf_type", "buf_name", "buf_body", "additional_info"
            ],

            "properties": {
                "version": {
                    "comment": "version of protocol",
                    "type": "string",
                    "const": "v1.1"
                },
                "id": {
                    "comment": "client id",
                    "type": "string"
                },
                "buf_type": {
                    "comment": "type of buffer entity",
                    "type": "string"
                },
                "buf_name": {
                    "comment": "name of buffer",
                    "type": "string"
                },
                "buf_body": {
                    "comment": "complete buffer entity",
                    "type": "string"
                },
                "additional_info": {
                    "comment": "some handler specific information",
                    "type": "string"
                }
            },
            "additionalProperties": false
        }
    }
}
)";

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

const std::string responseSchema_v11 = R"(
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
                    "enum": "v1.1"
                },
                "id": {
                    "comment": "client id",
                    "type": "string"
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
