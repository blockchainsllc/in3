
#include "../../core/client/keys.h"
#include "../../core/util/data.h"
#include "../../core/util/stringbuilder.h"

void sb_to_yaml(sb_t* sb, d_token_t* t, int level, bool as_array_item) {
  char* ind = NULL;
  if (level) {
    ind = alloca(level * 2 + 1);
    memset(ind, ' ', level * 2);
    ind[level * 2] = 0;
  }

  switch (d_type(t)) {
    case T_BYTES: {
      sb_printx(sb, " '%B'", d_bytes(t));
      return;
    }
    case T_STRING: {
      sb_printx(sb, " '%s'", d_string(t));
      return;
    }
    case T_INTEGER: {
      sb_printx(sb, " %u", d_int(t));
      return;
    }
    case T_NULL: {
      sb_add_chars(sb, " null");
      return;
    }
    case T_BOOLEAN: {
      sb_printx(sb, " %s", d_int(t) ? "true" : "false");
      return;
    }
    case T_ARRAY: {
      if (d_len(t) == 0) {
        sb_add_chars(sb, " []");
      }
      else {
        bool first = true;
        for_children_of(iter, t) {
          if (first && as_array_item)
            sb_add_chars(sb, "-");
          else
            sb_printx(sb, "\n%s-", ind ? ind : "");
          first = false;
          sb_to_yaml(sb, iter.token, level + 1, true);
        }
      }
      return;
    }
    case T_OBJECT: {
      bool first = true;
      for_children_of(iter, t) {
        bool skip  = false;
        bool after = false;
        for (d_iterator_t iter2 = d_iter(t); iter2.left; d_iter_next(&iter2)) {
          if (iter2.token == iter.token)
            after = true;
          else if (after && d_get_key(iter2.token) == d_get_key(iter.token)) {
            skip = true;
            break;
          }
        }
        if ((d_get_key(iter.token) == K_ID && d_get(t, key("jsonrpc"))) || d_get_key(iter.token) == key("jsonrpc")) continue;
        if (skip) continue;
        char* prop = d_get_property_name(t, ((d_token_internal_t*) iter.token)->key);
        if (first && as_array_item)
          sb_add_char(sb, ' ');
        else
          sb_printx(sb, "\n%s", ind ? ind : "");
        first = false;
        sb_printx(sb, "%s:", prop ? prop : "_");
        sb_to_yaml(sb, iter.token, level + 1, false);
        _free(prop);
      }
      return;
    }

    default:
      break;
  }
}