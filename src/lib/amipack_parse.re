/**
 * amiws -- Library with functions for read/create AMI packets
 * Copyright (C) 2016, Stas Kobzar <staskobzar@modulis.ca>
 *
 * This file is part of amiws.
 *
 * amiws is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * amiws is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with amiws.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file amipack_parse.c
 * @brief AMI (Asterisk Management Interface) packet parser.
 *
 * @author Stas Kobzar <stas.kobzar@modulis.ca>
 */

#include <stdio.h>
#include <string.h>
#include "amipack.h"

/**
 * Commands to run when known header parsed.
 * @param flag    Header type
 */
#define SET_HEADER(name)  len = cur - tok; \
                          hdr_name = strdup(name); \
                          goto yyc_key;

/**
 * Commands to run on Command AMI response header.
 * @param offset  Header name offset
 * @param flag    Header type
 */
#define CMD_HEADER(offset, name) len = cur - tok - offset; tok += offset; \
                          while(*tok == ' ') { tok++; len--; } \
                          len -= 2; \
                          char *val = strndup(tok, len); \
                          amipack_append (pack, strdup(name), strlen(name), val, len); \
                          tok = cur; goto yyc_command;

// introducing types:re2c for AMI packet
/*! re2c parcing conditions. */
enum yycond_pack {
  yyckey,
  yycvalue,
  yyccommand,
};

AMIPacket *amiparse_pack (const char *pack_str)
{
  AMIPacket *pack = amipack_init ();
  const char *marker = pack_str;
  const char *cur    = marker;
  const char *ctxmarker;
  int c = yyckey;
  int len = 0;

  const char *tok = marker;
  char *hdr_name;

/*!re2c
  re2c:define:YYCTYPE  = "unsigned char";
  re2c:define:YYCURSOR = "cur";
  re2c:define:YYMARKER = "marker";
  re2c:define:YYCTXMARKER = "ctxmarker";
  re2c:define:YYCONDTYPE = "yycond_pack";
  re2c:define:YYGETCONDITION = "c";
  re2c:define:YYGETCONDITION:naked = 1;
  re2c:define:YYSETCONDITION = "c = @@;";
  re2c:define:YYSETCONDITION:naked = 1;
  re2c:yyfill:enable = 0;

  CRLF              = "\r\n";
  END_COMMAND       = "--END COMMAND--";

  ACTION            = 'Action';
  EVENT             = 'Event';
  RESPONSE          = 'Response';

  PRIVILEGE         = 'Privilege';
  ACTIONID          = 'ActionID';
  MESSAGE           = 'Message';
  OUTPUT            = 'Output';

  <*> *     {
              if (hdr_name) free (hdr_name);
              amipack_destroy (pack);
              return NULL;
            }
  <key,value> CRLF CRLF { goto done; }

  <key> ":" " "* { tok = cur; goto yyc_value; }
  <key> ":" " "* CRLF / [a-zA-Z] {
              tok = cur;
              amipack_append(pack, hdr_name, strlen(hdr_name), strdup(""), 0);
              goto yyc_key;
            }
  <key> ":" " "* CRLF CRLF {
              tok = cur;
              amipack_append(pack, hdr_name, strlen(hdr_name), strdup(""), 0);
              goto done;
            }
  <key> RESPONSE ":" " "* 'Follows' CRLF {
              len = cur - tok;
              tok = cur;
              amipack_type (pack, AMI_RESPONSE);
              amipack_append(pack, strdup("Response"), 8, strdup("Follows"), 7);
              goto yyc_command;
            }
  <key> RESPONSE  {
              amipack_type (pack, AMI_RESPONSE);
              SET_HEADER("Response");
            }
  <key> ACTION {
              amipack_type (pack, AMI_ACTION);
              SET_HEADER("Action");
            }
  <key> EVENT  {
              amipack_type (pack, AMI_EVENT);
              SET_HEADER("Event");
            }

  <key> [^: ]+ {
              len = cur - tok;
              hdr_name = strndup(tok, len);
              goto yyc_key;
            }

  <value> CRLF / [a-zA-Z] { tok = cur; goto yyc_key; }
  <value> [^\r\n]* {
              len = cur - tok;
              char *val = strndup(tok, len);
              amipack_append(pack, hdr_name, strlen(hdr_name), val, len);
              goto yyc_value;
            }

  <command> PRIVILEGE ":" .* CRLF { CMD_HEADER(10, "Privilege"); }
  <command> ACTIONID ":" .* CRLF  { CMD_HEADER(9, "ActionID"); }
  <command> MESSAGE ":" .* CRLF   { CMD_HEADER(8, "Message"); }
  <command> OUTPUT ":" " "?       { tok = cur; goto yyc_command; }
  <command> .* "\r"? "\n"         { goto yyc_command; }
  <command> END_COMMAND CRLF CRLF {
              len = cur - tok - 19; // output minus command end tag
              char *val = strndup(tok, len);
              amipack_append (pack, strdup("Output"), 6, val, len);
              goto done;
            }
*/

done:
  return pack;
}