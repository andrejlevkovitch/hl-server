#pragma once

const char *request_schema_v11 = R"(
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

const char *response_schema_v11 = R"(
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
