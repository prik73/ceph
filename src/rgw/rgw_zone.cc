// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

#include <optional>

#include "common/errno.h"

#include "rgw_zone.h"
#include "rgw_sal.h"
#include "rgw_sal_config.h"
#include "rgw_sync.h"

#include "services/svc_zone.h"


#define dout_context g_ceph_context
#define dout_subsys ceph_subsys_rgw

namespace rgw_zone_defaults {

static std::string default_bucket_index_pool_suffix = "rgw.buckets.index";
static std::string default_storage_extra_pool_suffix = "rgw.buckets.non-ec";
static std::string zone_info_oid_prefix = "zone_info.";

std::string zone_names_oid_prefix = "zone_names.";
std::string region_info_oid_prefix = "region_info.";
std::string zone_group_info_oid_prefix = "zonegroup_info.";
std::string default_region_info_oid = "default.region";
std::string default_zone_group_info_oid = "default.zonegroup";
std::string region_map_oid = "region_map";
std::string default_zonegroup_name = "default";
std::string default_zone_name = "default";
std::string zonegroup_names_oid_prefix = "zonegroups_names.";
std::string RGW_DEFAULT_ZONE_ROOT_POOL = "rgw.root";
std::string RGW_DEFAULT_ZONEGROUP_ROOT_POOL = "rgw.root";
std::string RGW_DEFAULT_PERIOD_ROOT_POOL = "rgw.root";
std::string default_storage_pool_suffix = "rgw.buckets.data";

}

using namespace std;
using namespace rgw_zone_defaults;

void encode_json_plain(const char *name, const RGWAccessKey& val, Formatter *f)
{
  f->open_object_section(name);
  val.dump_plain(f);
  f->close_section();
}

static void decode_zones(map<rgw_zone_id, RGWZone>& zones, JSONObj *o)
{
  RGWZone z;
  z.decode_json(o);
  zones[z.id] = z;
}

static void decode_placement_targets(map<string, RGWZoneGroupPlacementTarget>& targets, JSONObj *o)
{
  RGWZoneGroupPlacementTarget t;
  t.decode_json(o);
  targets[t.name] = t;
}

void RGWZone::generate_test_instances(list<RGWZone*> &o)
{
  RGWZone *z = new RGWZone;
  o.push_back(z);
  o.push_back(new RGWZone);
}

void RGWZone::dump(Formatter *f) const
{
  encode_json("id", id, f);
  encode_json("name", name, f);
  encode_json("endpoints", endpoints, f);
  encode_json("log_meta", log_meta, f);
  encode_json("log_data", log_data, f);
  encode_json("bucket_index_max_shards", bucket_index_max_shards, f);
  encode_json("read_only", read_only, f);
  encode_json("tier_type", tier_type, f);
  encode_json("sync_from_all", sync_from_all, f);
  encode_json("sync_from", sync_from, f);
  encode_json("redirect_zone", redirect_zone, f);
  encode_json("supported_features", supported_features, f);
}

void RGWZone::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("id", id, obj);
  JSONDecoder::decode_json("name", name, obj);
  if (id.empty()) {
    id = name;
  }
  JSONDecoder::decode_json("endpoints", endpoints, obj);
  JSONDecoder::decode_json("log_meta", log_meta, obj);
  JSONDecoder::decode_json("log_data", log_data, obj);
  JSONDecoder::decode_json("bucket_index_max_shards", bucket_index_max_shards, obj);
  JSONDecoder::decode_json("read_only", read_only, obj);
  JSONDecoder::decode_json("tier_type", tier_type, obj);
  JSONDecoder::decode_json("sync_from_all", sync_from_all, true, obj);
  JSONDecoder::decode_json("sync_from", sync_from, obj);
  JSONDecoder::decode_json("redirect_zone", redirect_zone, obj);
  JSONDecoder::decode_json("supported_features", supported_features, obj);
}

rgw_pool RGWZoneGroup::get_pool(CephContext *cct_) const
{
  if (cct_->_conf->rgw_zonegroup_root_pool.empty()) {
    return rgw_pool(RGW_DEFAULT_ZONEGROUP_ROOT_POOL);
  }

  return rgw_pool(cct_->_conf->rgw_zonegroup_root_pool);
}

void RGWZoneGroup::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("id", id, obj);
  JSONDecoder::decode_json("name", name, obj);
  if (id.empty()) {
    derr << "old format " << dendl;
    JSONDecoder::decode_json("name", name, obj);
    id = name;
  }
  JSONDecoder::decode_json("api_name", api_name, obj);
  JSONDecoder::decode_json("is_master", is_master, obj);
  JSONDecoder::decode_json("endpoints", endpoints, obj);
  JSONDecoder::decode_json("hostnames", hostnames, obj);
  JSONDecoder::decode_json("hostnames_s3website", hostnames_s3website, obj);
  JSONDecoder::decode_json("master_zone", master_zone, obj);
  JSONDecoder::decode_json("zones", zones, decode_zones, obj);
  JSONDecoder::decode_json("placement_targets", placement_targets, decode_placement_targets, obj);
  string pr;
  JSONDecoder::decode_json("default_placement", pr, obj);
  default_placement.from_str(pr);
  JSONDecoder::decode_json("realm_id", realm_id, obj);
  JSONDecoder::decode_json("sync_policy", sync_policy, obj);
  JSONDecoder::decode_json("enabled_features", enabled_features, obj);
}

void RGWZoneParams::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("id", id, obj);
  JSONDecoder::decode_json("name", name, obj);
  JSONDecoder::decode_json("domain_root", domain_root, obj);
  JSONDecoder::decode_json("control_pool", control_pool, obj);
  JSONDecoder::decode_json("gc_pool", gc_pool, obj);
  JSONDecoder::decode_json("lc_pool", lc_pool, obj);
  JSONDecoder::decode_json("log_pool", log_pool, obj);
  JSONDecoder::decode_json("intent_log_pool", intent_log_pool, obj);
  JSONDecoder::decode_json("roles_pool", roles_pool, obj);
  JSONDecoder::decode_json("reshard_pool", reshard_pool, obj);
  JSONDecoder::decode_json("usage_log_pool", usage_log_pool, obj);
  JSONDecoder::decode_json("user_keys_pool", user_keys_pool, obj);
  JSONDecoder::decode_json("user_email_pool", user_email_pool, obj);
  JSONDecoder::decode_json("user_swift_pool", user_swift_pool, obj);
  JSONDecoder::decode_json("user_uid_pool", user_uid_pool, obj);
  JSONDecoder::decode_json("otp_pool", otp_pool, obj);
  JSONDecoder::decode_json("notif_pool", notif_pool, obj);
  JSONDecoder::decode_json("topics_pool", topics_pool, obj);
  JSONDecoder::decode_json("account_pool", account_pool, obj);
  JSONDecoder::decode_json("group_pool", group_pool, obj);
  JSONDecoder::decode_json("system_key", system_key, obj);
  JSONDecoder::decode_json("placement_pools", placement_pools, obj);
  JSONDecoder::decode_json("tier_config", tier_config, obj);
  JSONDecoder::decode_json("realm_id", realm_id, obj);
}

void RGWZoneParams::dump(Formatter *f) const
{
  encode_json("id", id, f);
  encode_json("name", name, f);
  encode_json("domain_root", domain_root, f);
  encode_json("control_pool", control_pool, f);
  encode_json("gc_pool", gc_pool, f);
  encode_json("lc_pool", lc_pool, f);
  encode_json("log_pool", log_pool, f);
  encode_json("intent_log_pool", intent_log_pool, f);
  encode_json("usage_log_pool", usage_log_pool, f);
  encode_json("roles_pool", roles_pool, f);
  encode_json("reshard_pool", reshard_pool, f);
  encode_json("user_keys_pool", user_keys_pool, f);
  encode_json("user_email_pool", user_email_pool, f);
  encode_json("user_swift_pool", user_swift_pool, f);
  encode_json("user_uid_pool", user_uid_pool, f);
  encode_json("otp_pool", otp_pool, f);
  encode_json("notif_pool", notif_pool, f);
  encode_json("topics_pool", topics_pool, f);
  encode_json("account_pool", account_pool, f);
  encode_json("group_pool", group_pool, f);
  encode_json_plain("system_key", system_key, f);
  encode_json("placement_pools", placement_pools, f);
  encode_json("tier_config", tier_config, f);
  encode_json("realm_id", realm_id, f);
}

rgw_pool RGWZoneParams::get_pool(CephContext *cct) const
{
  if (cct->_conf->rgw_zone_root_pool.empty()) {
    return rgw_pool(RGW_DEFAULT_ZONE_ROOT_POOL);
  }

  return rgw_pool(cct->_conf->rgw_zone_root_pool);
}

rgw_pool fix_zone_pool_dup(const set<rgw_pool>& pools,
                           const string& default_prefix,
                           const string& default_suffix,
                           const rgw_pool& suggested_pool)
{
  string suggested_name = suggested_pool.to_str();

  string prefix = default_prefix;
  string suffix = default_suffix;

  if (!suggested_pool.empty()) {
    prefix = suggested_name.substr(0, suggested_name.find("."));
    suffix = suggested_name.substr(prefix.length());
  }

  rgw_pool pool(prefix + suffix);
 
  while (pools.count(pool)) {
    pool = prefix + "_" + std::to_string(std::rand()) + suffix;
  }
  return pool;
}

void add_zone_pools(const RGWZoneParams& info,
                    std::set<rgw_pool>& pools)
{
  pools.insert(info.domain_root);
  pools.insert(info.control_pool);
  pools.insert(info.gc_pool);
  pools.insert(info.log_pool);
  pools.insert(info.intent_log_pool);
  pools.insert(info.usage_log_pool);
  pools.insert(info.user_keys_pool);
  pools.insert(info.user_email_pool);
  pools.insert(info.user_swift_pool);
  pools.insert(info.user_uid_pool);
  pools.insert(info.otp_pool);
  pools.insert(info.roles_pool);
  pools.insert(info.reshard_pool);
  pools.insert(info.oidc_pool);
  pools.insert(info.notif_pool);
  pools.insert(info.topics_pool);
  pools.insert(info.account_pool);
  pools.insert(info.group_pool);

  for (const auto& [pname, placement] : info.placement_pools) {
    pools.insert(placement.index_pool);
    for (const auto& [sname, sc] : placement.storage_classes.get_all()) {
      if (sc.data_pool) {
        pools.insert(sc.data_pool.get());
      }
    }
    pools.insert(placement.data_extra_pool);
  }
}

namespace rgw {

int get_zones_pool_set(const DoutPrefixProvider *dpp,
                       optional_yield y,
                       rgw::sal::ConfigStore* cfgstore,
                       std::string_view my_zone_id,
                       std::set<rgw_pool>& pools)
{
  std::array<std::string, 128> zone_names;
  rgw::sal::ListResult<std::string> listing;
  do {
    int r = cfgstore->list_zone_names(dpp, y, listing.next,
                                      zone_names, listing);
    if (r < 0) {
      ldpp_dout(dpp, 0) << "failed to list zones with " << cpp_strerror(r) << dendl;
      return r;
    }

    for (const auto& name : listing.entries) {
      RGWZoneParams info;
      r = cfgstore->read_zone_by_name(dpp, y, name, info, nullptr);
      if (r < 0) {
        ldpp_dout(dpp, 0) << "failed to load zone " << name
            << " with " << cpp_strerror(r) << dendl;
        return r;
      }
      if (info.get_id() != my_zone_id) {
        add_zone_pools(info, pools);
      }
    }
  } while (!listing.next.empty());

  return 0;
}

}

void RGWPeriodConfig::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("bucket_quota", quota.bucket_quota, obj);
  JSONDecoder::decode_json("user_quota", quota.user_quota, obj);
  JSONDecoder::decode_json("user_ratelimit", user_ratelimit, obj);
  JSONDecoder::decode_json("bucket_ratelimit", bucket_ratelimit, obj);
  JSONDecoder::decode_json("anonymous_ratelimit", anon_ratelimit, obj);
}

void RGWPeriodConfig::dump(Formatter *f) const
{
  encode_json("bucket_quota", quota.bucket_quota, f);
  encode_json("user_quota", quota.user_quota, f);
  encode_json("user_ratelimit", user_ratelimit, f);
  encode_json("bucket_ratelimit", bucket_ratelimit, f);
  encode_json("anonymous_ratelimit", anon_ratelimit, f);
}

void RGWZoneGroup::dump(Formatter *f) const
{
  encode_json("id", id , f);
  encode_json("name", name , f);
  encode_json("api_name", api_name, f);
  encode_json("is_master", is_master, f);
  encode_json("endpoints", endpoints, f);
  encode_json("hostnames", hostnames, f);
  encode_json("hostnames_s3website", hostnames_s3website, f);
  encode_json("master_zone", master_zone, f);
  encode_json_map("zones", zones, f); /* more friendly representation */
  encode_json_map("placement_targets", placement_targets, f); /* more friendly representation */
  encode_json("default_placement", default_placement, f);
  encode_json("realm_id", realm_id, f);
  encode_json("sync_policy", sync_policy, f);
  encode_json("enabled_features", enabled_features, f);
}

void RGWZoneGroupPlacementTarget::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("name", name, obj);
  JSONDecoder::decode_json("tags", tags, obj);
  JSONDecoder::decode_json("storage_classes", storage_classes, obj);
  if (storage_classes.empty()) {
    storage_classes.insert(RGW_STORAGE_CLASS_STANDARD);
  }
  JSONDecoder::decode_json("tier_targets", tier_targets, obj);
}

void RGWZonePlacementInfo::dump(Formatter *f) const
{
  encode_json("index_pool", index_pool, f);
  encode_json("storage_classes", storage_classes, f);
  encode_json("data_extra_pool", data_extra_pool, f);
  encode_json("index_type", (uint32_t)index_type, f);
  encode_json("inline_data", inline_data, f);

  /* no real need for backward compatibility of compression_type and data_pool in here,
   * rather not clutter the output */
}

void RGWZonePlacementInfo::generate_test_instances(list<RGWZonePlacementInfo*>& o)
{
  o.push_back(new RGWZonePlacementInfo);
  o.push_back(new RGWZonePlacementInfo);
  o.back()->index_pool = rgw_pool("rgw.buckets.index");
  
  o.back()->data_extra_pool = rgw_pool("rgw.buckets.non-ec");
  o.back()->index_type = rgw::BucketIndexType::Normal;
  o.back()->inline_data = false;
}

void RGWZonePlacementInfo::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("index_pool", index_pool, obj);
  JSONDecoder::decode_json("storage_classes", storage_classes, obj);
  JSONDecoder::decode_json("data_extra_pool", data_extra_pool, obj);
  uint32_t it;
  JSONDecoder::decode_json("index_type", it, obj);
  if(!JSONDecoder::decode_json("inline_data", inline_data, obj)) {
    inline_data = true;
  }
  index_type = (rgw::BucketIndexType)it;

  /* backward compatibility, these are now defined in storage_classes */
  string standard_compression_type;
  string *pcompression = nullptr;
  if (JSONDecoder::decode_json("compression", standard_compression_type, obj)) {
    pcompression = &standard_compression_type;
  }
  rgw_pool standard_data_pool;
  rgw_pool *ppool = nullptr;
  if (JSONDecoder::decode_json("data_pool", standard_data_pool, obj)) {
    ppool = &standard_data_pool;
  }
  if (ppool || pcompression) {
    storage_classes.set_storage_class(RGW_STORAGE_CLASS_STANDARD, ppool, pcompression);
  }
}

void RGWZoneGroupPlacementTarget::dump(Formatter *f) const
{
  encode_json("name", name, f);
  encode_json("tags", tags, f);
  encode_json("storage_classes", storage_classes, f);
  if (!tier_targets.empty()) {
    encode_json("tier_targets", tier_targets, f);
  }
}

void RGWZoneGroupPlacementTier::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("tier_type", tier_type, obj);
  JSONDecoder::decode_json("storage_class", storage_class, obj);
  JSONDecoder::decode_json("retain_head_object", retain_head_object, obj);
  if (is_tier_type_s3()) {
    JSONDecoder::decode_json("s3", t.s3, obj);
  }
  JSONDecoder::decode_json("allow_read_through", allow_read_through, obj);
  JSONDecoder::decode_json("read_through_restore_days", read_through_restore_days, obj);
  JSONDecoder::decode_json("restore_storage_class", restore_storage_class, obj);
  if (is_tier_type_s3_glacier()) {
    JSONDecoder::decode_json("s3-glacier", s3_glacier, obj);
  }
}

void RGWZoneStorageClasses::dump(Formatter *f) const
{
  for (auto& i : m) {
    encode_json(i.first.c_str(), i.second, f);
  }
}

void RGWZoneStorageClasses::generate_test_instances(list<RGWZoneStorageClasses*>& o)
{
  o.push_back(new RGWZoneStorageClasses);
}

void RGWZoneStorageClasses::decode_json(JSONObj *obj)
{
  JSONFormattable f;
  decode_json_obj(f, obj);

  for (auto& field : f.object()) {
    JSONObj *field_obj = obj->find_obj(field.first);
    assert(field_obj);

    decode_json_obj(m[field.first], field_obj);
  }
  standard_class = &m[RGW_STORAGE_CLASS_STANDARD];
}

void RGWZoneGroupTierS3Glacier::dump(Formatter *f) const
{
  encode_json("glacier_restore_days", glacier_restore_days, f);
  string s = (glacier_restore_tier_type == Standard ? "Standard" : "Expedited");
  encode_json("glacier_restore_tier_type", s, f);
}

void RGWZoneGroupTierS3Glacier::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("glacier_restore_days", glacier_restore_days, obj);
  string s;
  JSONDecoder::decode_json("glacier_restore_tier_type", s, obj);
  if (s != "Expedited") {
    glacier_restore_tier_type = Standard;
  } else {
    glacier_restore_tier_type = Expedited;
  }
}

void RGWZoneGroupPlacementTier::dump(Formatter *f) const
{
  encode_json("tier_type", tier_type, f);
  encode_json("storage_class", storage_class, f);
  encode_json("retain_head_object", retain_head_object, f);
  if (is_tier_type_s3()) {
    encode_json("s3", t.s3, f);
  }
  encode_json("allow_read_through", allow_read_through, f);
  encode_json("read_through_restore_days", read_through_restore_days, f);
  encode_json("restore_storage_class", restore_storage_class, f);

  if (is_tier_type_s3_glacier()) {
    encode_json("s3-glacier", s3_glacier, f);
  }
}

void RGWZoneGroupPlacementTierS3::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("endpoint", endpoint, obj);
  JSONDecoder::decode_json("access_key", key.id, obj);
  JSONDecoder::decode_json("secret", key.key, obj);
  JSONDecoder::decode_json("region", region, obj);
  string s;
  JSONDecoder::decode_json("host_style", s, obj);
  if (s != "virtual") {
    host_style = PathStyle;
  } else {
    host_style = VirtualStyle;
  }
  JSONDecoder::decode_json("target_storage_class", target_storage_class, obj);
  JSONDecoder::decode_json("target_path", target_path, obj);
  JSONDecoder::decode_json("acl_mappings", acl_mappings, obj);
  JSONDecoder::decode_json("multipart_sync_threshold", multipart_sync_threshold, obj);
  JSONDecoder::decode_json("multipart_min_part_size", multipart_min_part_size, obj);
}

void RGWZoneStorageClass::dump(Formatter *f) const
{
  if (data_pool) {
    encode_json("data_pool", data_pool.get(), f);
  }
  if (compression_type) {
    encode_json("compression_type", compression_type.get(), f);
  }
}

void RGWZoneStorageClass::generate_test_instances(list<RGWZoneStorageClass*>& o)
{
  o.push_back(new RGWZoneStorageClass);
  o.push_back(new RGWZoneStorageClass);
  o.back()->data_pool = rgw_pool("pool1");
  o.back()->compression_type = "zlib";
}

void RGWZoneStorageClass::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("data_pool", data_pool, obj);
  JSONDecoder::decode_json("compression_type", compression_type, obj);
}

void RGWTierACLMapping::decode_json(JSONObj *obj)
{
  string s;
  JSONDecoder::decode_json("type", s, obj);
  if (s == "email") {
    type = ACL_TYPE_EMAIL_USER;
  } else if (s == "uri") {
    type = ACL_TYPE_GROUP;
  } else {
    type = ACL_TYPE_CANON_USER;
  }

  JSONDecoder::decode_json("source_id", source_id, obj);
  JSONDecoder::decode_json("dest_id", dest_id, obj);
}

void RGWZoneGroupPlacementTierS3::dump(Formatter *f) const
{
  encode_json("endpoint", endpoint, f);
  encode_json("access_key", key.id, f);
  encode_json("secret", key.key, f);
  encode_json("region", region, f);
  string s = (host_style == PathStyle ? "path" : "virtual");
  encode_json("host_style", s, f);
  encode_json("target_storage_class", target_storage_class, f);
  encode_json("target_path", target_path, f);
  encode_json("acl_mappings", acl_mappings, f);
  encode_json("multipart_sync_threshold", multipart_sync_threshold, f);
  encode_json("multipart_min_part_size", multipart_min_part_size, f);
}

void RGWTierACLMapping::dump(Formatter *f) const
{
  string s;
  switch (type) {
    case ACL_TYPE_EMAIL_USER:
      s = "email";
      break;
    case ACL_TYPE_GROUP:
      s = "uri";
      break;
    default:
      s = "id";
      break;
  }
  encode_json("type", s, f);
  encode_json("source_id", source_id, f);
  encode_json("dest_id", dest_id, f);
}

void RGWPeriodMap::dump(Formatter *f) const
{
  encode_json("id", id, f);
  encode_json_map("zonegroups", zonegroups, f);
  encode_json("short_zone_ids", short_zone_ids, f);
}

static void decode_zonegroups(map<string, RGWZoneGroup>& zonegroups, JSONObj *o)
{
  RGWZoneGroup zg;
  zg.decode_json(o);
  zonegroups[zg.get_id()] = zg;
}

void RGWPeriodMap::decode_json(JSONObj *obj)
{
  JSONDecoder::decode_json("id", id, obj);
  JSONDecoder::decode_json("zonegroups", zonegroups, decode_zonegroups, obj);
  /* backward compatability with region */
  if (zonegroups.empty()) {
    JSONDecoder::decode_json("regions", zonegroups, obj);
  }
  /* backward compatability with region */
  if (master_zonegroup.empty()) {
    JSONDecoder::decode_json("master_region", master_zonegroup, obj);
  }
  JSONDecoder::decode_json("short_zone_ids", short_zone_ids, obj);
}

void RGWPeriodMap::decode(bufferlist::const_iterator& bl) {
  DECODE_START(2, bl);
  decode(id, bl);
  decode(zonegroups, bl);
  decode(master_zonegroup, bl);
  if (struct_v >= 2) {
    decode(short_zone_ids, bl);
  }
  DECODE_FINISH(bl);

  zonegroups_by_api.clear();
  for (map<string, RGWZoneGroup>::iterator iter = zonegroups.begin();
       iter != zonegroups.end(); ++iter) {
    RGWZoneGroup& zonegroup = iter->second;
    zonegroups_by_api[zonegroup.api_name] = zonegroup;
    if (zonegroup.is_master_zonegroup()) {
      master_zonegroup = zonegroup.get_id();
    }
  }
}

void RGWPeriodMap::encode(bufferlist& bl) const
{
  ENCODE_START(2, 1, bl);
  encode(id, bl);
  encode(zonegroups, bl);
  encode(master_zonegroup, bl);
  encode(short_zone_ids, bl);
  ENCODE_FINISH(bl);
}

bool RGWPeriodMap::find_zone_by_id(const rgw_zone_id& zone_id,
                                   RGWZoneGroup *zonegroup,
                                   RGWZone *zone) const
{
  for (auto& iter : zonegroups) {
    auto& zg = iter.second;

    auto ziter = zg.zones.find(zone_id);
    if (ziter != zg.zones.end()) {
      *zonegroup = zg;
      *zone = ziter->second;
      return true;
    }
  }

  return false;
}

namespace rgw {

int init_zone_pool_names(const DoutPrefixProvider *dpp, optional_yield y,
                         const std::set<rgw_pool>& pools, RGWZoneParams& info)
{
  info.domain_root = fix_zone_pool_dup(pools, info.name, ".rgw.meta:root", info.domain_root);
  info.control_pool = fix_zone_pool_dup(pools, info.name, ".rgw.control", info.control_pool);
  info.gc_pool = fix_zone_pool_dup(pools, info.name, ".rgw.log:gc", info.gc_pool);
  info.lc_pool = fix_zone_pool_dup(pools, info.name, ".rgw.log:lc", info.lc_pool);
  info.log_pool = fix_zone_pool_dup(pools, info.name, ".rgw.log", info.log_pool);
  info.intent_log_pool = fix_zone_pool_dup(pools, info.name, ".rgw.log:intent", info.intent_log_pool);
  info.usage_log_pool = fix_zone_pool_dup(pools, info.name, ".rgw.log:usage", info.usage_log_pool);
  info.user_keys_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:users.keys", info.user_keys_pool);
  info.user_email_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:users.email", info.user_email_pool);
  info.user_swift_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:users.swift", info.user_swift_pool);
  info.user_uid_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:users.uid", info.user_uid_pool);
  info.roles_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:roles", info.roles_pool);
  info.reshard_pool = fix_zone_pool_dup(pools, info.name, ".rgw.log:reshard", info.reshard_pool);
  info.otp_pool = fix_zone_pool_dup(pools, info.name, ".rgw.otp", info.otp_pool);
  info.oidc_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:oidc", info.oidc_pool);
  info.notif_pool = fix_zone_pool_dup(pools, info.name, ".rgw.log:notif", info.notif_pool);
  info.topics_pool =
      fix_zone_pool_dup(pools, info.name, ".rgw.meta:topics", info.topics_pool);
  info.account_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:accounts", info.account_pool);
  info.group_pool = fix_zone_pool_dup(pools, info.name, ".rgw.meta:groups", info.group_pool);

  for (auto& [pname, placement] : info.placement_pools) {
    placement.index_pool = fix_zone_pool_dup(pools, info.name, "." + default_bucket_index_pool_suffix, placement.index_pool);
    placement.data_extra_pool= fix_zone_pool_dup(pools, info.name, "." + default_storage_extra_pool_suffix, placement.data_extra_pool);
    for (auto& [sname, sc] : placement.storage_classes.get_all()) {
      if (sc.data_pool) {
        sc.data_pool = fix_zone_pool_dup(pools, info.name, "." + default_storage_pool_suffix, *sc.data_pool);
      }
    }
  }

  return 0;
}

std::string get_zonegroup_endpoint(const RGWZoneGroup& info)
{
  if (!info.endpoints.empty()) {
    return info.endpoints.front();
  }
  // use zonegroup's master zone endpoints
  auto z = info.zones.find(info.master_zone);
  if (z != info.zones.end() && !z->second.endpoints.empty()) {
    return z->second.endpoints.front();
  }
  return "";
}

int add_zone_to_group(const DoutPrefixProvider* dpp, RGWZoneGroup& zonegroup,
                      const RGWZoneParams& zone_params,
                      const bool *pis_master, const bool *pread_only,
                      const std::list<std::string>& endpoints,
                      const std::string *ptier_type,
                      const bool *psync_from_all,
                      const std::list<std::string>& sync_from,
                      const std::list<std::string>& sync_from_rm,
                      const std::string *predirect_zone,
                      std::optional<int> bucket_index_max_shards,
                      const rgw::zone_features::set& enable_features,
                      const rgw::zone_features::set& disable_features)
{
  const std::string& zone_id = zone_params.id;
  const std::string& zone_name = zone_params.name;

  if (zone_id.empty()) {
    ldpp_dout(dpp, -1) << __func__ << " requires a zone id" << dendl;
    return -EINVAL;
  }
  if (zone_name.empty()) {
    ldpp_dout(dpp, -1) << __func__ << " requires a zone name" << dendl;
    return -EINVAL;
  }

  // check for duplicate zone name on insert
  if (!zonegroup.zones.count(zone_id)) {
    for (const auto& [id, zone] : zonegroup.zones) {
      if (zone.name == zone_name) {
        ldpp_dout(dpp, 0) << "ERROR: found existing zone name " << zone_name
            << " (" << id << ") in zonegroup " << zonegroup.name << dendl;
        return -EEXIST;
      }
    }
  }

  rgw_zone_id& master_zone = zonegroup.master_zone;
  if (pis_master) {
    if (*pis_master) {
      if (!master_zone.empty() && master_zone != zone_id) {
        ldpp_dout(dpp, 0) << "NOTICE: overriding master zone: "
            << master_zone << dendl;
      }
      master_zone = zone_id;
    } else if (master_zone == zone_id) {
      master_zone.clear();
    }
  } else if (master_zone.empty() && zonegroup.zones.empty()) {
    ldpp_dout(dpp, 0) << "NOTICE: promoted " << zone_name
        << " as new master_zone of zonegroup " << zonegroup.name << dendl;
    master_zone = zone_id;
  }

  // make sure the zone's placement targets are named in the zonegroup
  for (const auto& [name, placement] : zone_params.placement_pools) {
    auto target = RGWZoneGroupPlacementTarget{.name = name};
    zonegroup.placement_targets.emplace(name, std::move(target));
  }

  RGWZone& zone = zonegroup.zones[zone_params.id];
  zone.id = zone_params.id;
  zone.name = zone_params.name;
  if (!endpoints.empty()) {
    zone.endpoints = endpoints;
  }
  if (pread_only) {
    zone.read_only = *pread_only;
  }
  if (ptier_type) {
    zone.tier_type = *ptier_type;
  }
  if (psync_from_all) {
    zone.sync_from_all = *psync_from_all;
  }
  if (predirect_zone) {
    zone.redirect_zone = *predirect_zone;
  }
  if (bucket_index_max_shards) {
    zone.bucket_index_max_shards = *bucket_index_max_shards;
  }

  // add/remove sync_from
  for (auto add : sync_from) {
    zone.sync_from.insert(add);
  }

  for (const auto& rm : sync_from_rm) {
    auto i = zone.sync_from.find(rm);
    if (i == zone.sync_from.end()) {
      ldpp_dout(dpp, 1) << "WARNING: zone \"" << rm
          << "\" was not in sync_from" << dendl;
      continue;
    }
    zone.sync_from.erase(i);
  }

  // add/remove supported features
  zone.supported_features.insert(enable_features.begin(),
                                 enable_features.end());

  for (const auto& feature : disable_features) {
    if (zonegroup.enabled_features.contains(feature)) {
      ldpp_dout(dpp, -1) << "ERROR: Cannot disable zone feature \"" << feature
          << "\" until it's been disabled in zonegroup " << zonegroup.name << dendl;
      return -EINVAL;
    }
    auto i = zone.supported_features.find(feature);
    if (i == zone.supported_features.end()) {
      ldpp_dout(dpp, 1) << "WARNING: zone feature \"" << feature
          << "\" was not enabled in zone " << zone.name << dendl;
      continue;
    }
    zone.supported_features.erase(i);
  }

  const bool log_data = zonegroup.zones.size() > 1;
  for (auto& [id, zone] : zonegroup.zones) {
    zone.log_data = log_data;
  }

  return 0;
}

} // namespace rgw

