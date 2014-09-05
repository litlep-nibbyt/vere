/* n/a.c
**
** This file is in the public domain.
*/
#include "all.h"

/* _box_slot(): select the right free list to search for a block.
*/
c3_w
_box_slot(c3_w siz_w)
{
  if ( siz_w < 8 ) {
    return 0;
  }
  else {
    c3_w i_w = 1;

    while ( 1 ) {
      if ( i_w == u2_cc_fbox_no ) {
        return (i_w - 1);
      }
      if ( siz_w < 16 ) {
        return i_w;
      }
      siz_w = (siz_w + 1) >> 1;
      i_w += 1;
    }
  }
}

/* _box_make(): construct a box.
*/
u2_cs_box*
_box_make(void* box_v, c3_w siz_w, c3_w use_w)
{
  u2_cs_box* box_u = box_v;
  c3_w*      box_w = box_v;

  box_w[0] = siz_w;
  box_w[siz_w - 1] = siz_w;
  box_u->use_w = use_w;

# ifdef  U2_MEMORY_DEBUG
    box_u->cod_w = COD_w;
# endif

  return box_u;
}

/* _box_attach(): attach a box to the free list.
*/
void
_box_attach(u2_cs_box* box_u)
{
  c3_assert(box_u->siz_w >= (1 + c3_wiseof(u2_cs_fbox)));

  {
    c3_w sel_w         = _box_slot(box_u->siz_w);
    u2_cs_fbox* fre_u  = (void *)box_u;
    u2_cs_fbox** pfr_u = &u2R->all.fre_u[sel_w];
    u2_cs_fbox* nex_u  = *pfr_u;

    fre_u->pre_u = 0;
    fre_u->nex_u = nex_u;
    if ( fre_u->nex_u ) {
      fre_u->nex_u->pre_u = fre_u;
    }
    (*pfr_u) = fre_u;
  }
}

/* _box_detach(): detach a box from the free list.
*/
void
_box_detach(u2_cs_box* box_u)
{
  u2_cs_fbox* fre_u = (void*) box_u;
  u2_cs_fbox* pre_u = fre_u->pre_u;
  u2_cs_fbox* nex_u = fre_u->nex_u;

  if ( nex_u ) {
    c3_assert(nex_u->pre_u == fre_u);
    nex_u->pre_u = pre_u;
  }
  if ( pre_u ) {
    c3_assert(pre_u->nex_u == fre_u);
    pre_u->nex_u = nex_u;
  }
  else {
    c3_w sel_w = _box_slot(box_u->siz_w);

    c3_assert(fre_u == u2R->all.fre_u[sel_w]);
    u2R->all.fre_u[sel_w] = nex_u;
  }
}

/* _me_road_all_hat(): in u2R, allocate directly on the hat_w.
*/
static c3_w*
_me_road_all_hat(c3_w len_w)
{
  if ( len_w > u2_co_open ) {
    u2_cm_bail(c3__meme); return 0;
  }

  if ( u2_yes == u2_co_is_north ) {
    c3_w* all_w;
     
    all_w = u2R->hat_w;
    u2R->hat_w += len_w;
    return all_w;
  }  
  else {
    u2R->hat_w -= len_w;
    return u2R->hat_w;
  }
}

#if 0  // not yet used
/* _me_road_all_cap(): in u2R, allocate directly on the cap.
*/
static c3_w*
_me_road_all_cap(c3_w len_w)
{
  if ( len_w > u2_co_open ) {
    u2_cm_bail(c3__meme); return 0;
  }

  if ( u2_yes == u2_co_is_north ) {
    u2R->cap_w -= len_w;
    return u2R->cap_w;
  }  
  else {
    c3_w* all_w;
     
    all_w = u2R->cap_w;
    u2R->cap_w += len_w;
    return all_w;
  }
}
#endif

/* u2_ca_walloc(): allocate storage words on hat_w.
*/
void*
u2_ca_walloc(c3_w len_w)
{
  c3_w siz_w = c3_max(u2_cc_minimum, u2_co_boxed(len_w));
  c3_w sel_w = _box_slot(siz_w);

  //  XX: this logic is totally bizarre, but preserve it.
  //
  if ( (sel_w != 0) && (sel_w != u2_cc_fbox_no - 1) ) {
    sel_w += 1;
  }

  while ( 1 ) {
    u2_cs_fbox** pfr_u = &u2R->all.fre_u[sel_w];

    while ( 1 ) {
      if ( 0 == *pfr_u ) {
        if ( sel_w < (u2_cc_fbox_no - 1) ) {
          sel_w += 1;
          break;
        }
        else {
          /* Nothing in top free list.  Chip away at the hat_w.
          */
          return u2_co_boxto(_box_make(_me_road_all_hat(siz_w), siz_w, 1));
        }
      }
      else {
        if ( siz_w > (*pfr_u)->box_u.siz_w ) {
          /* This free block is too small.  Continue searching.
          */
          pfr_u = &((*pfr_u)->nex_u);
          continue;
        } 
        else {
          u2_cs_box* box_u = &((*pfr_u)->box_u);

          /* We have found a free block of adequate size.  Remove it
          ** from the free list.
          */
          {
            {
              c3_assert((0 == (*pfr_u)->pre_u) || 
                        (*pfr_u)->pre_u->nex_u == (*pfr_u));
              c3_assert((0 == (*pfr_u)->nex_u) || 
                        (*pfr_u)->nex_u->pre_u == (*pfr_u));
            }
            if ( 0 != (*pfr_u)->nex_u ) {
              (*pfr_u)->nex_u->pre_u = (*pfr_u)->pre_u;
            }
            *pfr_u = (*pfr_u)->nex_u;
          }

          /* If we can chop off another block, do it.
          */
          if ( (siz_w + c3_wiseof(u2_cs_fbox) + 1) <= box_u->siz_w ) {
            /* Split the block.
            */ 
            c3_w* box_w = ((c3_w *)(void *)box_u);
            c3_w* end_w = box_w + siz_w;
            c3_w  lef_w = (box_u->siz_w - siz_w);

            _box_attach(_box_make(end_w, lef_w, 0));
            return u2_co_boxto(_box_make(box_w, siz_w, 1));
          }
          else {
            c3_assert(0 == box_u->use_w);
            box_u->use_w = 1;

#ifdef      U2_MEMORY_DEBUG
              box_u->cod_w = COD_w;
#endif
            return u2_co_boxto(box_u);
          }
        }
      }
    }
  }
}

/* u2_ca_malloc(): allocate storage measured in bytes.
*/
void*
u2_ca_malloc(c3_w len_w)
{
  return u2_ca_walloc((len_w + 3) >> 2);
}

/* u2_ca_wealloc(): realloc in words.
*/
void*
u2_ca_wealloc(void* lag_v, c3_w len_w)
{
  if ( !lag_v ) {
    return u2_ca_malloc(len_w);
  } 
  else {
    u2_cs_box* box_u = u2_co_botox(lag_v);
    c3_w*      old_w = lag_v;
    c3_w       tiz_w = c3_min(box_u->siz_w, len_w);
    {
      c3_w* new_w = u2_ca_walloc(len_w);
      c3_w  i_w;

      for ( i_w = 0; i_w < tiz_w; i_w++ ) {
        new_w[i_w] = old_w[i_w];
      }
      u2_ca_free(lag_v);
      return new_w;
    }
  }
}

/* u2_ca_realloc(): realloc in bytes.
*/
void*
u2_ca_realloc(void* lag_v, c3_w len_w)
{
  return u2_ca_wealloc(lag_v, (len_w + 3) >> 2);
}

/* u2_ca_free(): free storage.
*/
void
u2_ca_free(void* tox_v)
{
  u2_cs_box* box_u = u2_co_botox(tox_v);
  c3_w*      box_w = (c3_w *)(void *)box_u;

  c3_assert(box_u->use_w != 0);
  box_u->use_w -= 1;
  if ( 0 != box_u->use_w ) return;

  c3_assert(u2_yes == u2_co_is_north);
#if 0
  /* Clear the contents of the block, for debugging.
  */
  {
    c3_w i_w;

    for ( i_w = c3_wiseof(u2_cs_box); (i_w + 1) < box_u->siz_w; i_w++ ) {
      box_w[i_w] = 0xdeadbeef;
    }
  }
#endif

  if ( u2_yes == u2_co_is_north ) {
    /* Try to coalesce with the block below.
    */
    if ( box_w != u2R->rut_w ) {
      c3_w       laz_w = *(box_w - 1);
      u2_cs_box* pox_u = (u2_cs_box*)(void *)(box_w - laz_w);

      if ( 0 == pox_u->use_w ) {
        _box_detach(pox_u);
        _box_make(pox_u, (laz_w + box_u->siz_w), 0);

        box_u = pox_u;
        box_w = (c3_w*)(void *)pox_u;
      }
    }

    /* Try to coalesce with the block above, or the wilderness.
    */
    if ( (box_w + box_u->siz_w) == u2R->hat_w ) {
      u2R->hat_w = box_w;
    }
    else {
      u2_cs_box* nox_u = (u2_cs_box*)(void *)(box_w + box_u->siz_w);

      if ( 0 == nox_u->use_w ) {
        _box_detach(nox_u);
        _box_make(box_u, (box_u->siz_w + nox_u->siz_w), 0);
      }
      _box_attach(box_u);
    }
  }
  else {
    /* Try to coalesce with the block above.
    */
    if ( (box_w + box_u->siz_w) != u2R->rut_w ) {
      u2_cs_box* nox_u = (u2_cs_box*)(void *)(box_w + box_u->siz_w);

      if ( 0 == nox_u->use_w ) {
        _box_detach(nox_u);
        _box_make(box_u, (box_u->siz_w + nox_u->siz_w), 0);

        box_u = nox_u;
        box_w = (c3_w*)(void *)nox_u;
      }
    }

    /* Try to coalesce with the block below, or with the wilderness.
    */
    if ( box_w == u2R->hat_w ) {
      u2R->hat_w = (box_w + box_u->siz_w);
    }
    else {
      c3_w laz_w = *(box_w - 1);
      u2_cs_box* pox_u = (u2_cs_box*)(void *)(box_w - laz_w);

      if ( 0 == pox_u->use_w ) {
        _box_detach(pox_u);
        _box_make(pox_u, (laz_w + box_u->siz_w), 0);
      }
      _box_attach(box_u);
    }
  }
}

/* _me_wash_north(): clean up mug slots after copy.
*/
static void _me_wash_north(u2_noun dog);
static void
_me_wash_north_in(u2_noun som)
{
  if ( u2_so(u2_co_is_cat(som)) ) return;
  if ( u2_ne(u2_co_north_is_junior(som)) ) return;

  _me_wash_north(som);
}
static void
_me_wash_north(u2_noun dog)
{
  c3_assert(u2_co_is_dog(dog));
  c3_assert(u2_yes == u2_co_north_is_junior(dog));
  {
    u2_cs_noun* dog_u = u2_co_to_ptr(dog);

    if ( dog_u->mug_w >> 31 ) { dog_u->mug_w = 0; }

    if ( u2_so(u2_co_is_pom(dog)) ) {
      u2_cs_cell* god_u = (u2_cs_cell *)(void *)dog_u;
    
      _me_wash_north_in(god_u->hed);
      _me_wash_north_in(god_u->tel);
    }
  } 
}

/* _me_wash_south(): clean up mug slots after copy.
*/
static void _me_wash_south(u2_noun dog);
static void
_me_wash_south_in(u2_noun som)
{
  if ( u2_so(u2_co_is_cat(som)) ) return;
  if ( u2_ne(u2_co_south_is_junior(som)) ) return;

  _me_wash_south(som);
}
static void
_me_wash_south(u2_noun dog)
{
  c3_assert(u2_co_is_dog(dog));
  c3_assert(u2_yes == u2_co_south_is_junior(dog));
  {
    u2_cs_noun* dog_u = u2_co_to_ptr(dog);

    if ( dog_u->mug_w >> 31 ) { dog_u->mug_w = 0; }

    if ( u2_so(u2_co_is_pom(dog)) ) {
      u2_cs_cell* god_u = (u2_cs_cell *)(void *)dog_u;
    
      _me_wash_south_in(god_u->hed);
      _me_wash_south_in(god_u->tel);
    }
  } 
}

/* _me_gain_use(): increment use count.
*/
static void
_me_gain_use(u2_noun dog)
{
  c3_w* dog_w      = u2_co_to_ptr(dog);
  u2_cs_box* box_u = u2_co_botox(dog_w);

  if ( 0xffffffff == box_u->use_w ) {
    u2_cm_bail(c3__fail);
  }
  else {
    box_u->use_w += 1;
  }
}

/* _me_copy_north_in(): copy subjuniors on a north road.
*/
static u2_noun _me_copy_north(u2_noun);
static u2_noun
_me_copy_north_in(u2_noun som)
{
  c3_assert(u2_none != som);
  if ( u2_so(u2_co_is_cat(som)) ) {
    return som;
  }
  else { 
    u2_noun dog = som;

    if ( u2_so(u2_co_north_is_senior(dog)) ) {
      return dog;
    }
    else if ( u2_so(u2_co_north_is_junior(dog)) ) {
      return _me_copy_north(dog);
    }
    else {
      _me_gain_use(dog);
      return dog;
    }
  }
}
/* _me_copy_north(): copy juniors on a north road.
*/
static u2_noun
_me_copy_north(u2_noun dog)
{
  c3_assert(u2_yes == u2_co_north_is_junior(dog));

  if ( u2_ne(u2_co_north_is_junior(dog)) ) {
    if ( u2_ne(u2_co_north_is_senior(dog)) ) {
      _me_gain_use(dog);
    }
    return dog;
  } 
  else {
    u2_cs_noun* dog_u = u2_co_to_ptr(dog);

    /* Borrow mug slot to record new destination.
    */
    if ( dog_u->mug_w >> 31 ) {
      u2_noun nov = (u2_noun) dog_u->mug_w;

      c3_assert(u2_so(u2_co_north_is_normal(nov)));
      _me_gain_use(nov);

      return nov;
    }
    else {
      if ( u2_yes == u2_co_is_pom(dog) ) {
        u2_cs_cell* old_u = u2_co_to_ptr(dog);
        c3_w*       new_w = u2_ca_walloc(c3_wiseof(u2_cs_cell));
        u2_noun     new   = u2_co_de_twin(dog, new_w);
        u2_cs_cell* new_u = (u2_cs_cell*)(void *)new_w;

        new_u->mug_w = old_u->mug_w;
        new_u->hed = _me_copy_north_in(old_u->hed);
        new_u->tel = _me_copy_north_in(old_u->tel);

        /* Borrow mug slot to record new destination.
        */
        old_u->mug_w = new;
        return new;
      } 
      else {
        u2_cs_atom* old_u = u2_co_to_ptr(dog);
        c3_w*       new_w = u2_ca_walloc(old_u->len_w + c3_wiseof(u2_cs_atom));
        u2_noun     new   = u2_co_de_twin(dog, new_w);
        u2_cs_atom* new_u = (u2_cs_atom*)(void *)new_w;

        new_u->mug_w = old_u->mug_w;
        new_u->len_w = old_u->len_w;
        {
          c3_w i_w;

          for ( i_w=0; i_w < old_u->len_w; i_w++ ) {
            new_u->buf_w[i_w] = old_u->buf_w[i_w];
          }
        }

        /* Borrow mug slot to record new destination.
        */
        old_u->mug_w = new;
        return new;
      }
    }
  }
}

/* _me_copy_south_in(): copy subjuniors on a south road.
*/
static u2_noun _me_copy_south(u2_noun);
static u2_noun
_me_copy_south_in(u2_noun som)
{
  c3_assert(u2_none != som);
  if ( u2_so(u2_co_is_cat(som)) ) {
    return som;
  }
  else { 
    u2_noun dog = som;

    if ( u2_so(u2_co_south_is_senior(dog)) ) {
      return dog;
    }
    else if ( u2_so(u2_co_south_is_junior(dog)) ) {
      return _me_copy_south(dog);
    }
    else {
      _me_gain_use(dog);
      return dog;
    }
  }
}
/* _me_copy_south(): copy juniors on a south road.
*/
static u2_noun
_me_copy_south(u2_noun dog)
{
  c3_assert(u2_yes == u2_co_south_is_junior(dog));

  if ( u2_ne(u2_co_south_is_junior(dog)) ) {
    if ( u2_ne(u2_co_south_is_senior(dog)) ) {
      _me_gain_use(dog);
    }
    return dog;
  } 
  else {
    u2_cs_noun* dog_u = u2_co_to_ptr(dog);

    /* Borrow mug slot to record new destination.
    */
    if ( dog_u->mug_w >> 31 ) {
      u2_noun nov = (u2_noun) dog_u->mug_w;

      c3_assert(u2_so(u2_co_south_is_normal(nov)));
      _me_gain_use(nov);

      return nov;
    }
    else {
      if ( u2_yes == u2_co_is_pom(dog) ) {
        u2_cs_cell* old_u = u2_co_to_ptr(dog);
        c3_w*       new_w = u2_ca_walloc(c3_wiseof(u2_cs_cell));
        u2_noun     new   = u2_co_de_twin(dog, new_w);
        u2_cs_cell* new_u = (u2_cs_cell*)(void *)new_w;

        new_u->mug_w = old_u->mug_w;
        new_u->hed = _me_copy_south_in(old_u->hed);
        new_u->tel = _me_copy_south_in(old_u->tel);

        /* Borrow mug slot to record new destination.
        */
        old_u->mug_w = new;
        return new;
      } 
      else {
        u2_cs_atom* old_u = u2_co_to_ptr(dog);
        c3_w*       new_w = u2_ca_walloc(old_u->len_w + c3_wiseof(u2_cs_atom));
        u2_noun     new   = u2_co_de_twin(dog, new_w);
        u2_cs_atom* new_u = (u2_cs_atom*)(void *)new_w;

        new_u->mug_w = old_u->mug_w;
        new_u->len_w = old_u->len_w;
        {
          c3_w i_w;

          for ( i_w=0; i_w < old_u->len_w; i_w++ ) {
            new_u->buf_w[i_w] = old_u->buf_w[i_w];
          }
        }

        /* Borrow mug slot to record new destination.
        */
        old_u->mug_w = new;
        return new;
      }
    }
  }
}

/* _me_gain_north(): gain on a north road.
*/
static u2_noun
_me_gain_north(u2_noun dog)
{
  if ( u2_yes == u2_co_north_is_senior(dog) ) {
    /*  senior pointers are not refcounted
    */
    return dog;
  }
  else if ( u2_yes == u2_co_north_is_junior(dog) ) {
    /* junior pointers are copied
    */
    u2_noun mos = _me_copy_north(dog);

    _me_wash_north(dog);
    return mos;
  }
  else {
    /* normal pointers are refcounted
    */
    _me_gain_use(dog);
    return dog;
  }
}

/* _me_gain_south(): gain on a south road.
*/
static u2_noun
_me_gain_south(u2_noun dog)
{
  if ( u2_yes == u2_co_south_is_senior(dog) ) {
    /*  senior pointers are not refcounted
    */
    return dog;
  }
  else if ( u2_yes == u2_co_south_is_junior(dog) ) {
    /* junior pointers are copied
    */
    u2_noun mos = _me_copy_south(dog);

    _me_wash_south(dog);
    return mos;
  }
  else {
    /* normal pointers are refcounted
    */
    _me_gain_use(dog);
    return dog;
  }
}

/* _me_lose_north(): lose on a north road.
*/
static void
_me_lose_north(u2_noun dog)
{
top:
  if ( u2_yes == u2_co_north_is_normal(dog) ) {
    c3_w* dog_w      = u2_co_to_ptr(dog);
    u2_cs_box* box_u = u2_co_botox(dog_w);

    if ( box_u->use_w > 1 ) {
      box_u->use_w -= 1;
    }
    else {
      if ( 0 == box_u->use_w ) {
        u2_cm_bail(c3__foul);
      }
      else {
        if ( u2_so(u2_co_is_pom(dog)) ) {
          u2_cs_cell* dog_u = (void *)dog_w;
          u2_noun     h_dog = dog_u->hed;
          u2_noun     t_dog = dog_u->tel;

          if ( u2_ne(u2_co_is_cat(h_dog)) ) {
            _me_lose_north(h_dog);
          }
          u2_ca_free(dog_w);
          if ( u2_ne(u2_co_is_cat(t_dog)) ) {
            dog = t_dog;
            goto top;
          }
        }
        else {
          u2_ca_free(dog_w);
        }
      }
    }
  }
}

/* _me_lose_south(): lose on a south road.
*/
static void
_me_lose_south(u2_noun dog)
{
top:
  if ( u2_yes == u2_co_south_is_normal(dog) ) {
    c3_w* dog_w      = u2_co_to_ptr(dog);
    u2_cs_box* box_u = u2_co_botox(dog_w);

    if ( box_u->use_w > 1 ) {
      box_u->use_w -= 1;
    }
    else {
      if ( 0 == box_u->use_w ) {
        u2_cm_bail(c3__foul);
      }
      else {
        if ( u2_so(u2_co_is_pom(dog)) ) {
          u2_cs_cell* dog_u = (void *)dog_w;
          u2_noun     h_dog = dog_u->hed;
          u2_noun     t_dog = dog_u->tel;

          if ( u2_ne(u2_co_is_cat(h_dog)) ) {
            _me_lose_south(h_dog);
          }
          u2_ca_free(dog_w);
          if ( u2_ne(u2_co_is_cat(t_dog)) ) {
            dog = t_dog;
            goto top;
          }
        }
        else {
          u2_ca_free(dog_w);
        }
      }
    }
  }
}

/* u2_ca_gain(): gain a reference count, and/or copy juniors.
*/
u2_noun
u2_ca_gain(u2_noun som)
{
  c3_assert(u2_none != som);

  if ( u2_so(u2_co_is_cat(som)) ) {
    return som;
  }
  else {
    return u2_so(u2_co_is_north)
              ? _me_gain_north(som)
              : _me_gain_south(som);
  }
}

/* u2_ca_lose(): lose a reference count.
*/
void
u2_ca_lose(u2_noun som)
{
  if ( u2_ne(u2_co_is_cat(som)) ) {
    if ( u2_so(u2_co_is_north) ) {
      _me_lose_north(som);
    } else {
      _me_lose_south(som);
    }
  }
}

/* u2_ca_use(): reference count.
*/
c3_w
u2_ca_use(u2_noun som)
{
  if ( u2_so(u2_co_is_cat(som)) ) {
    return 1;
  } 
  else {
    c3_w* dog_w      = u2_co_to_ptr(som);
    u2_cs_box* box_u = u2_co_botox(dog_w);

    return box_u->use_w;
  }
}

/* u2_ca_slab(): create a length-bounded proto-atom.
*/
c3_w*
u2_ca_slab(c3_w len_w)
{
  c3_w*       nov_w = u2_ca_walloc(len_w + c3_wiseof(u2_cs_atom));
  u2_cs_atom* pug_u = (void *)nov_w;

  pug_u->mug_w = 0;
  pug_u->len_w = len_w;

  /* Clear teh slab.
  */
  {
    c3_w i_w;

    for ( i_w=0; i_w < len_w; i_w++ ) {
      pug_u->buf_w[i_w] = 0;
    }
  }
  return pug_u->buf_w;
}

/* u2_ca_slaq(): u2_ca_slaq() with a defined blocksize.
*/
c3_w*
u2_ca_slaq(c3_g met_g, c3_w len_w)
{
  return u2_ca_slab(((len_w << met_g) + 31) >> 5);
}

/* u2_ca_malt(): measure and finish a proto-atom.
*/
u2_noun
u2_ca_malt(c3_w* sal_w)
{
  c3_w*       nov_w = (sal_w - c3_wiseof(u2_cs_atom));
  u2_cs_atom* nov_u = (void *)nov_w;
  c3_w        len_w;

  for ( len_w = nov_u->len_w; len_w; len_w-- ) {
    if ( 0 != nov_u->buf_w[len_w - 1] ) {
      break;
    }
  }
  return u2_ca_mint(sal_w, len_w);
}

/* u2_ca_moot(): finish a pre-measured proto-atom; dangerous.
*/
u2_noun
u2_ca_moot(c3_w* sal_w)
{
  c3_w*       nov_w = (sal_w - c3_wiseof(u2_cs_atom));
  u2_cs_atom* nov_u = (void*)nov_w;
  c3_w        len_w = nov_u->len_w;
  c3_w        las_w = nov_u->buf_w[len_w - 1];

  c3_assert(0 != len_w);
  c3_assert(0 != las_w);

  if ( 1 == len_w ) {
    if ( u2_so(u2_co_is_cat(las_w)) ) {
      u2_ca_free(nov_w);

      return las_w;
    }
  }
  return u2_co_to_pug(u2_co_outa(nov_w));
}

/* u2_ca_mint(): finish a measured proto-atom.
*/
u2_noun
u2_ca_mint(c3_w* sal_w, c3_w len_w)
{
  c3_w*       nov_w = (sal_w - c3_wiseof(u2_cs_atom));
  u2_cs_atom* nov_u = (void*)nov_w;

  /* See if we can free the slab entirely.
  */
  if ( len_w == 0 ) {
    u2_ca_free(nov_w);

    return 0;
  }
  else if ( len_w == 1 ) {
    c3_w low_w = nov_u->buf_w[0];

    if ( u2_so(u2_co_is_cat(low_w)) ) {
      u2_ca_free(nov_w);

      return low_w;
    }
  }

  /* See if we can strip off a block on the end.
  */
  {
    c3_w old_w = nov_u->len_w;
    c3_w dif_w = (old_w - len_w);

    if ( dif_w >= u2_cc_minimum ) {
      c3_w* box_w = (void *)u2_co_botox(nov_w);
      c3_w* end_w = (nov_w + c3_wiseof(u2_cs_atom) + len_w + 1);
      c3_w  asz_w = (end_w - box_w);
      c3_w  bsz_w = box_w[0] - asz_w;

      _box_attach(_box_make(end_w, bsz_w, 0));

      box_w[0] = asz_w;
      box_w[asz_w - 1] = asz_w;
    }
    nov_u->len_w = len_w;
  }
  return u2_co_to_pug(u2_co_outa(nov_w));
}
