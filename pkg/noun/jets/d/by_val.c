/// @file

#include "jets/q.h"
#include "jets/w.h"
#include <stdio.h>

#include "noun.h"

//  [a] is RETAINED, [list] is TRANSFERRED
//
static u3_noun*
_by_val(u3_noun a, u3_noun* list)
{
  fprintf(stderr, "_by_val \r\n");
  if ( u3_nul == a ) {
    *list = u3_nul;
    return list;
  }
  else {
    u3_noun n_a, l_a, r_a;
    u3x_trel(a, &n_a, &l_a, &r_a);

    u3_noun*   hed;
    u3_noun*   tel;

    {
      *list = u3i_defcons(&hed, &tel);
      *hed = u3t(n_a);
      list = tel;
    }

    list = _by_val(l_a, list);

    return _by_val(r_a, list);
  }
}

u3_noun
u3qdb_val(u3_noun a)
{
  u3_noun empty = u3_nul;
  u3_noun* res = _by_val(a, &empty);
  return *res;
}

u3_noun
u3wdb_val(u3_noun cor)
{
  return u3qdb_val(u3x_at(u3x_con_sam, cor));
}
