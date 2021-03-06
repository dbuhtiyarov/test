/*
 * ====================================================================
 * Copyright (c) 2000-2002 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */
#include <ruby.h>
#include <svn_string.h>
#include <svn_pools.h>
#include <svn_wc.h>

#include "svn_ruby.h"
#include "wc.h"
#include "util.h"
#include "error.h"

static VALUE cSvnWcStatus, cSvnWcEntry;

typedef struct svn_ruby_wc_entry_t
{
  svn_wc_entry_t *entry;
  apr_pool_t *pool;
} svn_ruby_wc_entry_t;

typedef struct svn_ruby_wc_status_t
{
  svn_wc_status_t *status;
  apr_pool_t *pool;
} svn_ruby_wc_status_t;


/* Class methods. */

static VALUE
check_wc (VALUE self, VALUE aPath)
{
  svn_stringbuf_t *path;
  svn_boolean_t is_wc;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_check_wc (path, &is_wc, pool);

  apr_pool_destroy (pool);

  if (err)
    svn_ruby_raise (err);

  if (is_wc)
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
wc_has_binary_prop (VALUE self, VALUE aPath)
{
  svn_stringbuf_t *path;
  svn_boolean_t has_binary_prop;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_has_binary_prop (&has_binary_prop, path, pool);
  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  if (has_binary_prop)
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
text_modified_p (VALUE self, VALUE aFilename)
{
  svn_stringbuf_t *filename;
  svn_boolean_t modified_p;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aFilename, T_STRING);
  pool = svn_pool_create (NULL);
  filename = svn_stringbuf_create (StringValuePtr (aFilename), pool);
  err = svn_wc_text_modified_p (&modified_p, filename, pool);
  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  if (modified_p)
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
props_modified_p (VALUE self, VALUE aPath)
{
  svn_stringbuf_t *path;
  svn_boolean_t modified_p;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_props_modified_p (&modified_p, path, pool);
  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  if (modified_p)
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
prop_list (VALUE class, VALUE aPath)
{
  apr_hash_t *table;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  err = svn_wc_prop_list (&table, StringValuePtr (aPath), pool);
  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }
  
  {
    VALUE obj;
    apr_hash_index_t *hi;

    obj = rb_hash_new ();
    for (hi = apr_hash_first (pool, table); hi; hi = apr_hash_next (hi))
      {
        const void *key;
        void *val;
        apr_ssize_t key_len;
        svn_stringbuf_t *str;

        apr_hash_this (hi, &key, &key_len, &val);
        str = val;
        rb_hash_aset (obj, rb_str_new (key, key_len),
                      rb_str_new (str->data, str->len));
      }
    apr_pool_destroy (pool);
    return obj;
  }
}

static VALUE
wc_prop_get (VALUE class, VALUE aName, VALUE aPath)
{
  const svn_string_t *value;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aName, T_STRING);
  Check_Type (aPath, T_STRING);

  pool = svn_pool_create (NULL);
  err = svn_wc_prop_get (&value, StringValuePtr (aName),
                         StringValuePtr(aPath), pool);

  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  return rb_str_new (value->data, value->len);
}

static VALUE
wc_prop_set (VALUE class, VALUE aName, VALUE aValue, VALUE aPath)
{
  const svn_string_t *value;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aName, T_STRING);
  Check_Type (aValue, T_STRING);
  Check_Type (aPath, T_STRING);

  pool = svn_pool_create (NULL);
  value = svn_string_ncreate (StringValuePtr (aValue),
                              RSTRING (aValue)->len, pool);
  err = svn_wc_prop_set (StringValuePtr (aName), value,
                         StringValuePtr (aPath), pool);

  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  return rb_str_new (value->data, value->len);
}

static VALUE
is_wc_prop (VALUE class, VALUE aPath)
{
  svn_boolean_t wc_p;

  Check_Type (aPath, T_STRING);
  wc_p = svn_wc_is_wc_prop (StringValuePtr (aPath));

  if (wc_p)
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
wc_get_pristine_copy_path (VALUE class, VALUE aPath)
{
  svn_stringbuf_t *path, *pristine_path;
  apr_pool_t *pool;
  svn_error_t *err;
  VALUE obj;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_get_pristine_copy_path (path, &pristine_path, pool);

  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }
  obj = rb_str_new (pristine_path->data, pristine_path->len);
  apr_pool_destroy (pool);

  return obj;
}

static VALUE
wc_cleanup (VALUE class, VALUE aPath)
{
  svn_stringbuf_t *path;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_cleanup (path, pool);
  apr_pool_destroy (pool);

  if (err)
    svn_ruby_raise (err);

  return Qnil;
}

static VALUE
wc_revert (VALUE class, VALUE aPath, VALUE recursive)
{
  svn_stringbuf_t *path;
  apr_pool_t *pool;
  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_revert (path, RTEST (recursive), NULL, NULL, pool);
  apr_pool_destroy (pool);

  if (err)
    svn_ruby_raise (err);

  return Qnil;
}


/* WcEntry */

static void
free_wc_entry (void *p)
{
  svn_ruby_wc_entry_t *entry = p;
  long count = svn_ruby_get_refcount (entry->pool);
  if (count == 1)
    apr_pool_destroy (entry->pool);
  else
    svn_ruby_set_refcount (entry->pool, count - 1);
  free (entry);
}

static VALUE
wc_entry_new (VALUE class, svn_wc_entry_t *entry, apr_pool_t *pool)
{
  svn_ruby_wc_entry_t *rb_entry;
  VALUE obj = Data_Make_Struct (class, svn_ruby_wc_entry_t, 0, free_wc_entry,
                                rb_entry);
  rb_entry->entry = entry;
  rb_entry->pool = pool;

  return obj;
}


static VALUE
wc_entry_create (VALUE class, VALUE aPath)
{
  svn_wc_entry_t *entry;
  svn_stringbuf_t *path;
  apr_pool_t *pool;

  svn_error_t *err;
  VALUE obj;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_entry (&entry, path, pool);

  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  obj = wc_entry_new (class, entry, pool);
  svn_ruby_set_refcount (pool, 1);
  rb_iv_set (obj, "@path", aPath);

  return obj;
}

static VALUE
wc_entry_revision (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);

  return LONG2NUM (entry->entry->revision);
}

static VALUE
wc_entry_url (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  svn_stringbuf_t *url;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);
  url = entry->entry->url;

  if (!url)
    rb_raise (rb_eRuntimeError,
              "you need to create complete WcEntry object");

  return rb_str_new (url->data, url->len);
}

static VALUE
wc_entry_node_kind (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);

  return LONG2FIX (entry->entry->kind);
}

static VALUE
wc_entry_schedule (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);

  return LONG2FIX (entry->entry->schedule);
}

static VALUE
wc_entry_conflicted (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);

  if (entry->entry->conflicted)
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
wc_entry_copied (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);

  if (entry->entry->copied)
    return Qtrue;
  else
    return Qfalse;
}

static VALUE
wc_entry_text_time (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  apr_time_t text_time;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);

  text_time = entry->entry->text_time;
  if (!text_time)
    return Qnil;
  else
    return rb_time_new (text_time / APR_USEC_PER_SEC,
                        text_time % APR_USEC_PER_SEC);
}

static VALUE
wc_entry_prop_time (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  apr_time_t prop_time;
  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);

  prop_time = entry->entry->prop_time;
  if (!prop_time)
    return Qnil;
  else
    return rb_time_new (prop_time / APR_USEC_PER_SEC,
                        prop_time % APR_USEC_PER_SEC);
}

static VALUE
wc_entry_attributes (VALUE self)
{
  svn_ruby_wc_entry_t *entry;
  apr_hash_t *attributes;

  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);
  attributes = entry->entry->attributes;

  if (!attributes)
    rb_raise (rb_eRuntimeError,
              "you need to create complete WcEntry object");

  {
    VALUE obj;
    apr_pool_t *pool;
    apr_hash_index_t *hi;

    pool = svn_pool_create (NULL);
    obj = rb_hash_new ();
    for (hi = apr_hash_first (pool, attributes); hi; hi = apr_hash_next (hi))
      {
        const void *key;
        void *val;
        apr_ssize_t key_len;
        svn_stringbuf_t *value;

        apr_hash_this (hi, &key, &key_len, &val);
        value = (svn_stringbuf_t *) val;
        rb_hash_aset (obj, rb_str_new (key, key_len),
                      rb_str_new (value->data, value->len));
      }
    apr_pool_destroy (pool);
    return obj;
  }
}

static VALUE
wc_entry_entries_read (VALUE class, VALUE aPath)
{
  apr_hash_t *entries;
  svn_stringbuf_t *path;
  apr_pool_t *pool;

  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_entries_read (&entries, path, pool);

  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  {
    VALUE obj;
    apr_hash_index_t *hi;
    long count = 0;
    apr_pool_t *subpool = svn_pool_create (pool);

    obj = rb_hash_new ();
    for (hi = apr_hash_first (subpool, entries); hi; hi = apr_hash_next (hi))
      {
        const void *key;
        void *val;
        apr_ssize_t key_len;
        svn_wc_entry_t *entry;

        apr_hash_this (hi, &key, &key_len, &val);
        entry = (svn_wc_entry_t *) val;
        count++;
        rb_hash_aset (obj, rb_str_new (key, key_len),
                      wc_entry_new (class, entry, pool));
      }

    svn_ruby_set_refcount (pool, count);
    apr_pool_destroy (subpool);
    return obj;
  }
}

static VALUE
wc_entry_conflicted_p (VALUE self)
{
  svn_boolean_t text_conflicted_p, prop_conflicted_p;
  svn_stringbuf_t *dir_path;
  apr_pool_t *pool;
  svn_error_t *err;

  svn_ruby_wc_entry_t *entry;
  VALUE obj, aPath;

  Data_Get_Struct (self, svn_ruby_wc_entry_t, entry);
  pool = svn_pool_create (NULL);
  aPath = rb_iv_get (self, "@path");
  /* Should we use svn function or ruby method? */
  if (entry->entry->kind == svn_node_file)
    aPath = rb_funcall (rb_cFile, rb_intern ("dirname"), 1, aPath);
  dir_path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  err = svn_wc_conflicted_p (&text_conflicted_p, &prop_conflicted_p,
                             dir_path, entry->entry, pool);

  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  obj = rb_ary_new2 (2);
  rb_ary_store (obj, 0, text_conflicted_p ? Qtrue : Qfalse);
  rb_ary_store (obj, 1, prop_conflicted_p ? Qtrue : Qfalse);
  apr_pool_destroy (pool);
  return obj;
}


/* WcStatus */

static void
free_wc_status (void *p)
{
  svn_ruby_wc_status_t *status = p;
  long count = svn_ruby_get_refcount (status->pool);

  if (count == 1)
    apr_pool_destroy (status->pool);
  else
    svn_ruby_set_refcount (status->pool, count - 1);
  free (status);
}

static VALUE
wc_status_new (svn_wc_status_t *status, apr_pool_t *pool)
{
  VALUE obj;
  svn_ruby_wc_status_t *rb_status;

  obj = Data_Make_Struct (cSvnWcStatus, svn_ruby_wc_status_t, 0, free_wc_status,
                          rb_status);
  rb_status->status = status;
  rb_status->pool = pool;

  return obj;
}

static VALUE
wc_status (VALUE class, VALUE aPath)
{
  svn_wc_status_t *status;
  svn_stringbuf_t *path;
  apr_pool_t *pool;

  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);

  err = svn_wc_status (&status, path, pool);

  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  svn_ruby_set_refcount (pool, 1);
  return wc_status_new (status, pool);
}

/* Public interface.  client.c will use this.

  Convert STATUSHASH to Ruby hash table.  Be aware that all entry
  shares the same pool.  To correctly manage memory, you must
  reference count that pool.
*/

VALUE
svn_ruby_wc_to_statuses (apr_hash_t *statushash, apr_pool_t *pool)
{
  VALUE obj;
  apr_hash_index_t *hi;
  apr_pool_t *subpool;
  long count = 0;

  subpool = svn_pool_create (pool);
  obj = rb_hash_new ();
  for (hi = apr_hash_first (subpool, statushash); hi; hi = apr_hash_next (hi))
    {
      const void *key;
      void *val;
      apr_ssize_t key_len;
      svn_wc_status_t *status;

      apr_hash_this (hi, &key, &key_len, &val);
      status = (svn_wc_status_t *) val;
      count++;
      rb_hash_aset (obj, rb_str_new (key, key_len),
                    wc_status_new (status, pool));
    }

  svn_ruby_set_refcount (pool, count);
  apr_pool_destroy (subpool);
  return obj;
}

static VALUE
wc_statuses (VALUE class, VALUE aPath, VALUE descend, VALUE get_all, VALUE strict)
{
  apr_hash_t *statushash;
  svn_stringbuf_t *path;
  apr_pool_t *pool;

  svn_error_t *err;

  Check_Type (aPath, T_STRING);
  pool = svn_pool_create (NULL);
  path = svn_stringbuf_create (StringValuePtr (aPath), pool);
  statushash = apr_hash_make (pool);

  err = svn_wc_statuses (statushash, path,
                         RTEST (descend), RTEST (get_all),
                         RTEST (strict), pool);
  if (err)
    {
      apr_pool_destroy (pool);
      svn_ruby_raise (err);
    }

  return svn_ruby_wc_to_statuses (statushash, pool);
}

static VALUE
wc_status_entry (VALUE self)
{
  svn_ruby_wc_status_t *status;

  Data_Get_Struct (self, svn_ruby_wc_status_t, status);

  if (status->status->entry == NULL)
    return Qnil;
  else
    {
      VALUE obj;
      long count;

      obj = wc_entry_new (cSvnWcEntry, status->status->entry, status->pool);
      count = svn_ruby_get_refcount (status->pool);
      svn_ruby_set_refcount (status->pool, count + 1);

      return obj;
    }
}

static VALUE
wc_status_text_status (VALUE self)
{
  svn_ruby_wc_status_t *status;

  Data_Get_Struct (self, svn_ruby_wc_status_t, status);
  return INT2FIX (status->status->text_status);
}

static VALUE
wc_status_prop_status (VALUE self)
{
  svn_ruby_wc_status_t *status;

  Data_Get_Struct (self, svn_ruby_wc_status_t, status);
  return INT2FIX (status->status->prop_status);
}

static VALUE
wc_status_is_locked (VALUE self)
{
  svn_ruby_wc_status_t *status;

  Data_Get_Struct (self, svn_ruby_wc_status_t, status);
  return status->status->locked ? Qtrue : Qfalse;
}

static VALUE
wc_status_repos_text_status (VALUE self)
{
  svn_ruby_wc_status_t *status;

  Data_Get_Struct (self, svn_ruby_wc_status_t, status);
  return INT2FIX (status->status->repos_text_status);
}

static VALUE
wc_status_repos_prop_status (VALUE self)
{
  svn_ruby_wc_status_t *status;

  Data_Get_Struct (self, svn_ruby_wc_status_t, status);
  return INT2FIX (status->status->repos_prop_status);
}


static void
define_prop (VALUE module, const char *name, const char *value)
{
  rb_define_const (module, name, rb_str_new2 (value));
}

void svn_ruby_init_wc (void)
{
  VALUE cSvnWc;
  cSvnWc = rb_define_class_under (svn_ruby_mSvn, "Wc", rb_cObject);
  rb_undef_method (CLASS_OF (cSvnWc), "new");
  define_prop (cSvnWc, "ADM_DIR_NAME", SVN_WC_ADM_DIR_NAME);
  rb_define_singleton_method (cSvnWc, "wc?", check_wc, 1);
  rb_define_singleton_method (cSvnWc, "hasBinaryProp?", wc_has_binary_prop, 1);
  rb_define_singleton_method (cSvnWc, "textModified?", text_modified_p, 1);
  rb_define_singleton_method (cSvnWc, "propsModified?", props_modified_p, 1);
  rb_define_singleton_method (cSvnWc, "proplist", prop_list, 1);
  rb_define_singleton_method (cSvnWc, "propGet", wc_prop_get, 2);
  rb_define_singleton_method (cSvnWc, "propSet", wc_prop_set, 3);
  rb_define_singleton_method (cSvnWc, "wcProp?", is_wc_prop, 1);
  rb_define_singleton_method (cSvnWc, "getPristineCopyPath",
			      wc_get_pristine_copy_path, 1);
  rb_define_singleton_method (cSvnWc, "cleanup", wc_cleanup, 1);
  rb_define_singleton_method (cSvnWc, "revert", wc_revert, 2);

  cSvnWcEntry = rb_define_class_under (svn_ruby_mSvn, "WcEntry", rb_cObject);
  rb_define_singleton_method (cSvnWcEntry, "new", wc_entry_create, 1);
  rb_define_singleton_method (cSvnWcEntry, "entries", wc_entry_entries_read, 1);
  rb_define_const (cSvnWcEntry, "SCHEDULE_NORMAL",
                   INT2NUM (svn_wc_schedule_normal));
  rb_define_const (cSvnWcEntry, "SCHEDULE_ADD",
                   INT2NUM (svn_wc_schedule_add));
  rb_define_const (cSvnWcEntry, "SCHEDULE_DELETE",
                   INT2NUM (svn_wc_schedule_delete));
  rb_define_const (cSvnWcEntry, "SCHEDULE_REPLACE",
                   INT2NUM (svn_wc_schedule_replace));
  define_prop (cSvnWcEntry, "THIS_DIR",  SVN_WC_ENTRY_THIS_DIR);
  rb_define_method (cSvnWcEntry, "revision", wc_entry_revision, 0);
  rb_define_method (cSvnWcEntry, "url", wc_entry_url, 0);
  rb_define_method (cSvnWcEntry, "kind", wc_entry_node_kind, 0);
  rb_define_method (cSvnWcEntry, "schedule", wc_entry_schedule, 0);
  rb_define_method (cSvnWcEntry, "conflict?", wc_entry_conflicted, 0);
  rb_define_method (cSvnWcEntry, "copied?", wc_entry_copied, 0);
  rb_define_method (cSvnWcEntry, "textTime", wc_entry_text_time, 0);
  rb_define_method (cSvnWcEntry, "propTime", wc_entry_prop_time, 0);
  rb_define_method (cSvnWcEntry, "attributes", wc_entry_attributes, 0);
  rb_define_method (cSvnWcEntry, "conflicted?", wc_entry_conflicted_p, 0);
  cSvnWcStatus = rb_define_class_under (svn_ruby_mSvn, "WcStatus", rb_cObject);
  rb_define_singleton_method (cSvnWcStatus, "new", wc_status, 1);
  rb_define_singleton_method (cSvnWcStatus, "statuses", wc_statuses, 4);
  rb_define_const (cSvnWcStatus, "NONE", INT2FIX (svn_wc_status_none));
  rb_define_const (cSvnWcStatus, "UNVERSIONED", INT2FIX (svn_wc_status_unversioned));
  rb_define_const (cSvnWcStatus, "NORMAL", INT2FIX (svn_wc_status_normal));
  rb_define_const (cSvnWcStatus, "ADDED", INT2FIX (svn_wc_status_added));
  rb_define_const (cSvnWcStatus, "ABSENT", INT2FIX (svn_wc_status_absent));
  rb_define_const (cSvnWcStatus, "DELETED", INT2FIX (svn_wc_status_deleted));
  rb_define_const (cSvnWcStatus, "REPLACED", INT2FIX (svn_wc_status_replaced));
  rb_define_const (cSvnWcStatus, "MODIFIED", INT2FIX (svn_wc_status_modified));
  rb_define_const (cSvnWcStatus, "MERGED", INT2FIX (svn_wc_status_merged));
  rb_define_const (cSvnWcStatus, "CONFLICTED", INT2FIX (svn_wc_status_conflicted));
  rb_define_method (cSvnWcStatus, "entry", wc_status_entry, 0);
  rb_define_method (cSvnWcStatus, "textStatus", wc_status_text_status, 0);
  rb_define_method (cSvnWcStatus, "propStatus", wc_status_prop_status, 0);
  rb_define_method (cSvnWcStatus, "locked?", wc_status_is_locked, 0);
  rb_define_method (cSvnWcStatus, "reposTextStatus",
		    wc_status_repos_text_status, 0);
  rb_define_method (cSvnWcStatus, "reposPropStatus",
		    wc_status_repos_prop_status, 0);
}
