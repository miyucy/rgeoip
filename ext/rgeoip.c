#include "ruby.h"
#include <GeoIP.h>
#include <GeoIPCity.h>

/* -------- */

typedef int (*GeoIP_id_by) (GeoIP*, const char *);
typedef GeoIPRecord* (*GeoIP_record_by) (GeoIP*, const char *);

/* -------- */

enum {
    RS_CODE,
    RS_CODE3,
    RS_NAME,
    RS_REGION,
    RS_CITY,
    RS_POSTAL_CODE,
    RS_LATITUDE,
    RS_LONGITUDE,
    RS_DMA_CODE,
    RS_AREA_CODE,
    RGEOIP_SYMBOLS_MAX,
} RGEOIP_SYMBOLS;

/* -------- */

static VALUE rb_cRgeoip, rb_cRgeoipDatabase;
static VALUE rgeoip_symbols[RGEOIP_SYMBOLS_MAX];

/* -------- */

typedef struct {
    VALUE databases;
} Rgeoip;

typedef struct {
    GeoIP *ptr;
} RgeoipDatabase;

/* -------- */

static VALUE
gi_alloc(VALUE klass);
static void
gi_mark(Rgeoip* gi);
static void
gi_free(Rgeoip* gi);
static VALUE
gi_new(int argc, VALUE *argv, VALUE self);
static VALUE
gi_open(VALUE self, VALUE filenames);
static VALUE
gi_country(VALUE self, VALUE addr_or_host);
static VALUE
gi_city(VALUE self, VALUE addr_or_host);
static VALUE
gid_alloc(VALUE klass);
static void
gid_mark(RgeoipDatabase* gid);
static void
gid_free(RgeoipDatabase* gid);
static VALUE
gid_country(VALUE self, VALUE addr_or_host);
static VALUE
gid_city(VALUE self, VALUE addr_or_host);
static GeoIP_id_by
GeoIP_id_dwim(VALUE addr);
static GeoIP_record_by
GeoIP_record_dwim(VALUE addr);
static int
is_addr_p(VALUE value);
static VALUE
rb_hash_sset(VALUE hash, VALUE key, VALUE value);

/* Rgeoip */
/*
 * Document-class: Rgeoip
 *   * libGeoIP binding
 */

static VALUE
gi_alloc(VALUE klass)
{
    return Data_Wrap_Struct(klass, gi_mark, gi_free, 0);
}

static void
gi_mark(Rgeoip* gi)
{
    rb_gc_mark(gi->databases);
}

static void
gi_free(Rgeoip* gi)
{
    xfree(gi);
}

/*
 * Document-method: new
 * call-seq:
 *   Rgeoip.new
 *   Rgeoip.new("/path/to/database")
 *   Rgeoip.new("/path/to/database", "/path/to/other_database")
 *   Rgeoip.new(["/path/to/database", "/path/to/other_database"])
 *
 * @param path to GeoIP.dat and/or GeoLiteCity.dat
 * @raise [RuntimeError] Problem opening database
 * @return [Rgeoip] Rgeoip instance
 */
static VALUE
gi_new(int argc, VALUE *argv, VALUE self)
{
    Rgeoip      *gi;
    VALUE     flags;
    VALUE filenames;
    int           i;

    Data_Get_Struct(self, Rgeoip, gi);
    if (gi == NULL)
    {
        gi = ALLOC(Rgeoip);
        MEMZERO(gi, Rgeoip, 1);
        DATA_PTR(self) = gi;
    }

    gi->databases = rb_ary_new();

    for (i=0; i<argc; i++)
    {
        gi_open(self, argv[i]);
    }

    return self;
}

static VALUE
_gi_open(VALUE filenames, VALUE self)
{
    return gi_open(self, filenames);
}

/*
 * Document-method: open
 * call-seq:
 *   Rgeoip.open("/path/to/database")
 *   Rgeoip.open("/path/to/database", "/path/to/other_database")
 *   Rgeoip.open(["/path/to/database", "/path/to/other_database"])
 *
 * @param path to GeoIP.dat and/or GeoLiteCity.dat
 * @raise [RuntimeError] Problem opening database
 * @return [Rgeoip] Rgeoip instance
 */
static VALUE
gi_open(VALUE self, VALUE filenames)
{
    Rgeoip *gi;
    VALUE args[1] = {0};
    Data_Get_Struct(self, Rgeoip, gi);

    if (TYPE(filenames) == T_ARRAY)
    {
        rb_iterate(rb_each, filenames, _gi_open, self);
    }
    else
    {
        args[0] = filenames;
        rb_ary_push(gi->databases, rb_class_new_instance(1, args, rb_cRgeoipDatabase));
    }

    return self;
}

/*
 * Document-method: country
 * call-seq:
 *   rgeoip.country("example.com")
 *   rgeoip.country("127.0.0.1")
 *
 * @param [String] addr_or_host ipaddress or hostname
 * @return Hash object or nil
 */
static VALUE
gi_country(VALUE self, VALUE addr_or_host)
{
    Rgeoip          *gi;
    RgeoipDatabase *gid;
    int               i;
    VALUE        result;

    Data_Get_Struct(self, Rgeoip, gi);
    for(i=0; i<RARRAY_LEN(gi->databases); i++)
    {
        Data_Get_Struct(RARRAY_PTR(gi->databases)[i], RgeoipDatabase, gid);
        if (gid->ptr->databaseType != GEOIP_COUNTRY_EDITION)
        {
            continue;
        }

        result = gid_country(RARRAY_PTR(gi->databases)[i], addr_or_host);
        if (!NIL_P(result))
        {
            return result;
        }
    }

    return Qnil;
}

/*
 * Document-method: city
 * call-seq:
 *   rgeoip.city("example.com")
 *   rgeoip.city("127.0.0.1")
 *
 * @param [String] addr_or_host ipaddress or hostname
 * @return Hash object or nil
 */
static VALUE
gi_city(VALUE self, VALUE addr_or_host)
{
    Rgeoip          *gi;
    RgeoipDatabase *gid;
    int               i;
    VALUE        result;

    Data_Get_Struct(self, Rgeoip, gi);
    for(i=0; i<RARRAY_LEN(gi->databases); i++)
    {
        Data_Get_Struct(RARRAY_PTR(gi->databases)[i], RgeoipDatabase, gid);
        if (gid->ptr->databaseType != GEOIP_CITY_EDITION_REV0 &&
            gid->ptr->databaseType != GEOIP_CITY_EDITION_REV1)
        {
            continue;
        }

        result = gid_city(RARRAY_PTR(gi->databases)[i], addr_or_host);
        if (!NIL_P(result))
        {
            return result;
        }
    }

    return Qnil;
}

/* Rgeoip::Database */
/*
 * Document-class: Rgeoip::Database
 *   * GeoIP Container
 */

static VALUE
gid_alloc(VALUE klass)
{
    return Data_Wrap_Struct(klass, gid_mark, gid_free, 0);
}

static void
gid_mark(RgeoipDatabase* gid)
{
}

static void
gid_free(RgeoipDatabase* gid)
{
    GeoIP_delete(gid->ptr);
    xfree(gid);
}

/*
 * Document-method: new
 * call-seq:
 *   Rgeoip::Database.new "/path/to/dbfile"
 *
 * @param [String] filename path to GeoIP.dat or GeoLiteCity.dat
 * @raise [RuntimeError] Problem opening database
 * @return [Rgeoip::Database] Rgeoip::Database instance
 */
static VALUE
gid_new(int argc, VALUE *argv, VALUE self)
{
    RgeoipDatabase *gid;
    VALUE filename, flags;

    Data_Get_Struct(self, RgeoipDatabase, gid);
    if (gid == NULL)
    {
        gid = ALLOC(RgeoipDatabase);
        MEMZERO(gid, RgeoipDatabase, 1);
        DATA_PTR(self) = gid;
    }

    rb_scan_args(argc, argv, "11", &filename, &flags);

    Check_Type(filename, T_STRING);

    gid->ptr = GeoIP_open(RSTRING_PTR(filename),
                          GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE | GEOIP_MMAP_CACHE);
    if (gid->ptr == NULL)
    {
        rb_raise(rb_eRuntimeError, "Problem opening database");
    }

    return self;
}

/*
 * Document-method: country
 * call-seq:
 *   db.country("example.com")
 *   db.country("127.0.0.1")
 *
 * @param [String] addr_or_host ipaddress or hostname
 * @return Hash object or nil
 */
static VALUE
gid_country(VALUE self, VALUE addr_or_host)
{
    RgeoipDatabase *gid;
    GeoIP_id_by     any = GeoIP_id_dwim(addr_or_host);
    int              id;
    VALUE        result;

    if (NIL_P(addr_or_host))
    {
        return Qnil;
    }

    Data_Get_Struct(self, RgeoipDatabase, gid);

    id = (*any)(gid->ptr, RSTRING_PTR(addr_or_host));
    if (id == 0)
    {
        return Qnil;
    }

    result = rb_hash_new();
    if (GeoIP_code_by_id(id))
    {
        rb_hash_sset(result, rgeoip_symbols[RS_CODE], rb_str_freeze(rb_str_new2(GeoIP_code_by_id(id))));
    }
    if (GeoIP_code3_by_id(id))
    {
        rb_hash_sset(result, rgeoip_symbols[RS_CODE3], rb_str_freeze(rb_str_new2(GeoIP_code3_by_id(id))));
    }
    if (GeoIP_name_by_id(id))
    {
        rb_hash_sset(result, rgeoip_symbols[RS_NAME], rb_str_freeze(rb_str_new2(GeoIP_name_by_id(id))));
    }

    return result;
}

/*
 * Document-method: city
 * call-seq:
 *   db.city("example.com")
 *   db.city("127.0.0.1")
 *
 * @param [String] addr_or_host ipaddress or hostname
 * @return Hash object or nil
 */
static VALUE
gid_city(VALUE self, VALUE addr_or_host)
{
    RgeoipDatabase *gid;
    GeoIP_record_by any = GeoIP_record_dwim(addr_or_host);
    GeoIPRecord *record;
    VALUE        result;

    if (NIL_P(addr_or_host))
    {
        return Qnil;
    }

    Data_Get_Struct(self, RgeoipDatabase, gid);

    record = (*any)(gid->ptr, RSTRING_PTR(addr_or_host));
    if (record == NULL)
    {
        return Qnil;
    }

    result = rb_hash_new();
    if (record->country_code)
    {
        rb_hash_sset(result, rgeoip_symbols[RS_CODE], rb_str_freeze(rb_str_new2(record->country_code)));
    }
    if (record->country_code3)
    {
        rb_hash_sset(result, rgeoip_symbols[RS_CODE3], rb_str_freeze(rb_str_new2(record->country_code3)));
    }
    if (record->country_name)
    {
        rb_hash_sset(result, rgeoip_symbols[RS_NAME], rb_str_freeze(rb_str_new2(record->country_name)));
    }
    if (record->region)
    {
        rb_hash_sset(result, rgeoip_symbols[RS_REGION], rb_str_freeze(rb_str_new2(record->region)));
    }
    if (record->city)
    {
        rb_hash_sset(result, rgeoip_symbols[RS_CITY], rb_str_freeze(rb_str_new2(record->city)));
    }
    if (record->postal_code)
    {
        rb_hash_sset(result, rgeoip_symbols[RS_POSTAL_CODE], rb_str_freeze(rb_str_new2(record->postal_code)));
    }
    rb_hash_sset(result, rgeoip_symbols[RS_LATITUDE],  rb_float_new((double)record->latitude));
    rb_hash_sset(result, rgeoip_symbols[RS_LONGITUDE], rb_float_new((double)record->longitude));
    rb_hash_sset(result, rgeoip_symbols[RS_DMA_CODE],  INT2NUM(record->dma_code));
    rb_hash_sset(result, rgeoip_symbols[RS_AREA_CODE], INT2NUM(record->area_code));
    GeoIPRecord_delete(record);

    return result;
}

/* -------- */

static GeoIP_id_by
GeoIP_id_dwim(VALUE addr)
{
    if (NIL_P(addr))
    {
        return NULL;
    }
    if (is_addr_p(addr))
    {
        return GeoIP_id_by_addr;
    }
    return GeoIP_id_by_name;
}

static GeoIP_record_by
GeoIP_record_dwim(VALUE addr)
{
    if (NIL_P(addr))
    {
        return NULL;
    }
    if (is_addr_p(addr))
    {
        return GeoIP_record_by_addr;
    }
    return GeoIP_record_by_name;
}

static int
is_addr_p(VALUE value)
{
    int a,b,c,d;
    char      e;

    Check_Type(value, T_STRING);
    if (4 <= sscanf(RSTRING_PTR(value), "%d.%d.%d.%d%c", &a, &b, &c, &d, &e))
    {
        return 1;
    }
    return 0;
}

static VALUE
rb_hash_sset(VALUE hash, VALUE key, VALUE value)
{
    return rb_hash_aset(hash, key, value);
}

/* -------- */

void
Init_rgeoip(void)
{
    rb_cRgeoip = rb_define_class("Rgeoip", rb_cObject);
    rb_define_alloc_func(rb_cRgeoip, gi_alloc);
    rb_define_method(rb_cRgeoip, "initialize", gi_new, -1);
    rb_define_method(rb_cRgeoip, "open", gi_open, 1);
    rb_define_method(rb_cRgeoip, "country", gi_country, 1);
    rb_define_method(rb_cRgeoip, "city", gi_city, 1);

    rb_cRgeoipDatabase = rb_define_class_under(rb_cRgeoip, "Database", rb_cObject);
    rb_define_alloc_func(rb_cRgeoipDatabase, gid_alloc);
    rb_define_method(rb_cRgeoipDatabase, "initialize", gid_new, -1);
    rb_define_method(rb_cRgeoipDatabase, "country", gid_country, 1);
    rb_define_method(rb_cRgeoipDatabase, "city", gid_city, 1);

    rgeoip_symbols[RS_CODE]        = ID2SYM(rb_intern("code"));
    rgeoip_symbols[RS_CODE3]       = ID2SYM(rb_intern("code3"));
    rgeoip_symbols[RS_NAME]        = ID2SYM(rb_intern("name"));
    rgeoip_symbols[RS_REGION]      = ID2SYM(rb_intern("region"));
    rgeoip_symbols[RS_CITY]        = ID2SYM(rb_intern("city"));
    rgeoip_symbols[RS_POSTAL_CODE] = ID2SYM(rb_intern("postal_code"));
    rgeoip_symbols[RS_LATITUDE]    = ID2SYM(rb_intern("latitude"));
    rgeoip_symbols[RS_LONGITUDE]   = ID2SYM(rb_intern("longitude"));
    rgeoip_symbols[RS_DMA_CODE]    = ID2SYM(rb_intern("dma_code"));
    rgeoip_symbols[RS_AREA_CODE]   = ID2SYM(rb_intern("area_code"));
}
