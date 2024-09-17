// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: mq.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_mq_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_mq_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021002 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_mq_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_mq_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_mq_2eproto;
namespace jukey {
namespace prot {
class MqMsg;
struct MqMsgDefaultTypeInternal;
extern MqMsgDefaultTypeInternal _MqMsg_default_instance_;
}  // namespace prot
}  // namespace jukey
PROTOBUF_NAMESPACE_OPEN
template<> ::jukey::prot::MqMsg* Arena::CreateMaybeMessage<::jukey::prot::MqMsg>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace jukey {
namespace prot {

// ===================================================================

class MqMsg final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:jukey.prot.MqMsg) */ {
 public:
  inline MqMsg() : MqMsg(nullptr) {}
  ~MqMsg() override;
  explicit PROTOBUF_CONSTEXPR MqMsg(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  MqMsg(const MqMsg& from);
  MqMsg(MqMsg&& from) noexcept
    : MqMsg() {
    *this = ::std::move(from);
  }

  inline MqMsg& operator=(const MqMsg& from) {
    CopyFrom(from);
    return *this;
  }
  inline MqMsg& operator=(MqMsg&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const MqMsg& default_instance() {
    return *internal_default_instance();
  }
  static inline const MqMsg* internal_default_instance() {
    return reinterpret_cast<const MqMsg*>(
               &_MqMsg_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(MqMsg& a, MqMsg& b) {
    a.Swap(&b);
  }
  inline void Swap(MqMsg* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(MqMsg* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  MqMsg* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<MqMsg>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const MqMsg& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const MqMsg& from) {
    MqMsg::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(MqMsg* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "jukey.prot.MqMsg";
  }
  protected:
  explicit MqMsg(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kInstanceIdFieldNumber = 2,
    kExchangeFieldNumber = 3,
    kRoutingKeyFieldNumber = 4,
    kUserDataFieldNumber = 5,
    kTraceDataFieldNumber = 6,
    kExtendDataFieldNumber = 7,
    kServiceTypeFieldNumber = 1,
  };
  // required string instance_id = 2;
  bool has_instance_id() const;
  private:
  bool _internal_has_instance_id() const;
  public:
  void clear_instance_id();
  const std::string& instance_id() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_instance_id(ArgT0&& arg0, ArgT... args);
  std::string* mutable_instance_id();
  PROTOBUF_NODISCARD std::string* release_instance_id();
  void set_allocated_instance_id(std::string* instance_id);
  private:
  const std::string& _internal_instance_id() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_instance_id(const std::string& value);
  std::string* _internal_mutable_instance_id();
  public:

  // optional string exchange = 3;
  bool has_exchange() const;
  private:
  bool _internal_has_exchange() const;
  public:
  void clear_exchange();
  const std::string& exchange() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_exchange(ArgT0&& arg0, ArgT... args);
  std::string* mutable_exchange();
  PROTOBUF_NODISCARD std::string* release_exchange();
  void set_allocated_exchange(std::string* exchange);
  private:
  const std::string& _internal_exchange() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_exchange(const std::string& value);
  std::string* _internal_mutable_exchange();
  public:

  // optional string routing_key = 4;
  bool has_routing_key() const;
  private:
  bool _internal_has_routing_key() const;
  public:
  void clear_routing_key();
  const std::string& routing_key() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_routing_key(ArgT0&& arg0, ArgT... args);
  std::string* mutable_routing_key();
  PROTOBUF_NODISCARD std::string* release_routing_key();
  void set_allocated_routing_key(std::string* routing_key);
  private:
  const std::string& _internal_routing_key() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_routing_key(const std::string& value);
  std::string* _internal_mutable_routing_key();
  public:

  // optional string user_data = 5;
  bool has_user_data() const;
  private:
  bool _internal_has_user_data() const;
  public:
  void clear_user_data();
  const std::string& user_data() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_user_data(ArgT0&& arg0, ArgT... args);
  std::string* mutable_user_data();
  PROTOBUF_NODISCARD std::string* release_user_data();
  void set_allocated_user_data(std::string* user_data);
  private:
  const std::string& _internal_user_data() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_user_data(const std::string& value);
  std::string* _internal_mutable_user_data();
  public:

  // optional string trace_data = 6;
  bool has_trace_data() const;
  private:
  bool _internal_has_trace_data() const;
  public:
  void clear_trace_data();
  const std::string& trace_data() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_trace_data(ArgT0&& arg0, ArgT... args);
  std::string* mutable_trace_data();
  PROTOBUF_NODISCARD std::string* release_trace_data();
  void set_allocated_trace_data(std::string* trace_data);
  private:
  const std::string& _internal_trace_data() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_trace_data(const std::string& value);
  std::string* _internal_mutable_trace_data();
  public:

  // optional string extend_data = 7;
  bool has_extend_data() const;
  private:
  bool _internal_has_extend_data() const;
  public:
  void clear_extend_data();
  const std::string& extend_data() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_extend_data(ArgT0&& arg0, ArgT... args);
  std::string* mutable_extend_data();
  PROTOBUF_NODISCARD std::string* release_extend_data();
  void set_allocated_extend_data(std::string* extend_data);
  private:
  const std::string& _internal_extend_data() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_extend_data(const std::string& value);
  std::string* _internal_mutable_extend_data();
  public:

  // required uint32 service_type = 1;
  bool has_service_type() const;
  private:
  bool _internal_has_service_type() const;
  public:
  void clear_service_type();
  uint32_t service_type() const;
  void set_service_type(uint32_t value);
  private:
  uint32_t _internal_service_type() const;
  void _internal_set_service_type(uint32_t value);
  public:

  // @@protoc_insertion_point(class_scope:jukey.prot.MqMsg)
 private:
  class _Internal;

  // helper for ByteSizeLong()
  size_t RequiredFieldsByteSizeFallback() const;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr instance_id_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr exchange_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr routing_key_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr user_data_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr trace_data_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr extend_data_;
    uint32_t service_type_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_mq_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// MqMsg

// required uint32 service_type = 1;
inline bool MqMsg::_internal_has_service_type() const {
  bool value = (_impl_._has_bits_[0] & 0x00000040u) != 0;
  return value;
}
inline bool MqMsg::has_service_type() const {
  return _internal_has_service_type();
}
inline void MqMsg::clear_service_type() {
  _impl_.service_type_ = 0u;
  _impl_._has_bits_[0] &= ~0x00000040u;
}
inline uint32_t MqMsg::_internal_service_type() const {
  return _impl_.service_type_;
}
inline uint32_t MqMsg::service_type() const {
  // @@protoc_insertion_point(field_get:jukey.prot.MqMsg.service_type)
  return _internal_service_type();
}
inline void MqMsg::_internal_set_service_type(uint32_t value) {
  _impl_._has_bits_[0] |= 0x00000040u;
  _impl_.service_type_ = value;
}
inline void MqMsg::set_service_type(uint32_t value) {
  _internal_set_service_type(value);
  // @@protoc_insertion_point(field_set:jukey.prot.MqMsg.service_type)
}

// required string instance_id = 2;
inline bool MqMsg::_internal_has_instance_id() const {
  bool value = (_impl_._has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool MqMsg::has_instance_id() const {
  return _internal_has_instance_id();
}
inline void MqMsg::clear_instance_id() {
  _impl_.instance_id_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000001u;
}
inline const std::string& MqMsg::instance_id() const {
  // @@protoc_insertion_point(field_get:jukey.prot.MqMsg.instance_id)
  return _internal_instance_id();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void MqMsg::set_instance_id(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000001u;
 _impl_.instance_id_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:jukey.prot.MqMsg.instance_id)
}
inline std::string* MqMsg::mutable_instance_id() {
  std::string* _s = _internal_mutable_instance_id();
  // @@protoc_insertion_point(field_mutable:jukey.prot.MqMsg.instance_id)
  return _s;
}
inline const std::string& MqMsg::_internal_instance_id() const {
  return _impl_.instance_id_.Get();
}
inline void MqMsg::_internal_set_instance_id(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000001u;
  _impl_.instance_id_.Set(value, GetArenaForAllocation());
}
inline std::string* MqMsg::_internal_mutable_instance_id() {
  _impl_._has_bits_[0] |= 0x00000001u;
  return _impl_.instance_id_.Mutable(GetArenaForAllocation());
}
inline std::string* MqMsg::release_instance_id() {
  // @@protoc_insertion_point(field_release:jukey.prot.MqMsg.instance_id)
  if (!_internal_has_instance_id()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000001u;
  auto* p = _impl_.instance_id_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.instance_id_.IsDefault()) {
    _impl_.instance_id_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void MqMsg::set_allocated_instance_id(std::string* instance_id) {
  if (instance_id != nullptr) {
    _impl_._has_bits_[0] |= 0x00000001u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000001u;
  }
  _impl_.instance_id_.SetAllocated(instance_id, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.instance_id_.IsDefault()) {
    _impl_.instance_id_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:jukey.prot.MqMsg.instance_id)
}

// optional string exchange = 3;
inline bool MqMsg::_internal_has_exchange() const {
  bool value = (_impl_._has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool MqMsg::has_exchange() const {
  return _internal_has_exchange();
}
inline void MqMsg::clear_exchange() {
  _impl_.exchange_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000002u;
}
inline const std::string& MqMsg::exchange() const {
  // @@protoc_insertion_point(field_get:jukey.prot.MqMsg.exchange)
  return _internal_exchange();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void MqMsg::set_exchange(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000002u;
 _impl_.exchange_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:jukey.prot.MqMsg.exchange)
}
inline std::string* MqMsg::mutable_exchange() {
  std::string* _s = _internal_mutable_exchange();
  // @@protoc_insertion_point(field_mutable:jukey.prot.MqMsg.exchange)
  return _s;
}
inline const std::string& MqMsg::_internal_exchange() const {
  return _impl_.exchange_.Get();
}
inline void MqMsg::_internal_set_exchange(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000002u;
  _impl_.exchange_.Set(value, GetArenaForAllocation());
}
inline std::string* MqMsg::_internal_mutable_exchange() {
  _impl_._has_bits_[0] |= 0x00000002u;
  return _impl_.exchange_.Mutable(GetArenaForAllocation());
}
inline std::string* MqMsg::release_exchange() {
  // @@protoc_insertion_point(field_release:jukey.prot.MqMsg.exchange)
  if (!_internal_has_exchange()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000002u;
  auto* p = _impl_.exchange_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.exchange_.IsDefault()) {
    _impl_.exchange_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void MqMsg::set_allocated_exchange(std::string* exchange) {
  if (exchange != nullptr) {
    _impl_._has_bits_[0] |= 0x00000002u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000002u;
  }
  _impl_.exchange_.SetAllocated(exchange, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.exchange_.IsDefault()) {
    _impl_.exchange_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:jukey.prot.MqMsg.exchange)
}

// optional string routing_key = 4;
inline bool MqMsg::_internal_has_routing_key() const {
  bool value = (_impl_._has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool MqMsg::has_routing_key() const {
  return _internal_has_routing_key();
}
inline void MqMsg::clear_routing_key() {
  _impl_.routing_key_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000004u;
}
inline const std::string& MqMsg::routing_key() const {
  // @@protoc_insertion_point(field_get:jukey.prot.MqMsg.routing_key)
  return _internal_routing_key();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void MqMsg::set_routing_key(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000004u;
 _impl_.routing_key_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:jukey.prot.MqMsg.routing_key)
}
inline std::string* MqMsg::mutable_routing_key() {
  std::string* _s = _internal_mutable_routing_key();
  // @@protoc_insertion_point(field_mutable:jukey.prot.MqMsg.routing_key)
  return _s;
}
inline const std::string& MqMsg::_internal_routing_key() const {
  return _impl_.routing_key_.Get();
}
inline void MqMsg::_internal_set_routing_key(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000004u;
  _impl_.routing_key_.Set(value, GetArenaForAllocation());
}
inline std::string* MqMsg::_internal_mutable_routing_key() {
  _impl_._has_bits_[0] |= 0x00000004u;
  return _impl_.routing_key_.Mutable(GetArenaForAllocation());
}
inline std::string* MqMsg::release_routing_key() {
  // @@protoc_insertion_point(field_release:jukey.prot.MqMsg.routing_key)
  if (!_internal_has_routing_key()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000004u;
  auto* p = _impl_.routing_key_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.routing_key_.IsDefault()) {
    _impl_.routing_key_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void MqMsg::set_allocated_routing_key(std::string* routing_key) {
  if (routing_key != nullptr) {
    _impl_._has_bits_[0] |= 0x00000004u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000004u;
  }
  _impl_.routing_key_.SetAllocated(routing_key, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.routing_key_.IsDefault()) {
    _impl_.routing_key_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:jukey.prot.MqMsg.routing_key)
}

// optional string user_data = 5;
inline bool MqMsg::_internal_has_user_data() const {
  bool value = (_impl_._has_bits_[0] & 0x00000008u) != 0;
  return value;
}
inline bool MqMsg::has_user_data() const {
  return _internal_has_user_data();
}
inline void MqMsg::clear_user_data() {
  _impl_.user_data_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000008u;
}
inline const std::string& MqMsg::user_data() const {
  // @@protoc_insertion_point(field_get:jukey.prot.MqMsg.user_data)
  return _internal_user_data();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void MqMsg::set_user_data(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000008u;
 _impl_.user_data_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:jukey.prot.MqMsg.user_data)
}
inline std::string* MqMsg::mutable_user_data() {
  std::string* _s = _internal_mutable_user_data();
  // @@protoc_insertion_point(field_mutable:jukey.prot.MqMsg.user_data)
  return _s;
}
inline const std::string& MqMsg::_internal_user_data() const {
  return _impl_.user_data_.Get();
}
inline void MqMsg::_internal_set_user_data(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000008u;
  _impl_.user_data_.Set(value, GetArenaForAllocation());
}
inline std::string* MqMsg::_internal_mutable_user_data() {
  _impl_._has_bits_[0] |= 0x00000008u;
  return _impl_.user_data_.Mutable(GetArenaForAllocation());
}
inline std::string* MqMsg::release_user_data() {
  // @@protoc_insertion_point(field_release:jukey.prot.MqMsg.user_data)
  if (!_internal_has_user_data()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000008u;
  auto* p = _impl_.user_data_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.user_data_.IsDefault()) {
    _impl_.user_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void MqMsg::set_allocated_user_data(std::string* user_data) {
  if (user_data != nullptr) {
    _impl_._has_bits_[0] |= 0x00000008u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000008u;
  }
  _impl_.user_data_.SetAllocated(user_data, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.user_data_.IsDefault()) {
    _impl_.user_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:jukey.prot.MqMsg.user_data)
}

// optional string trace_data = 6;
inline bool MqMsg::_internal_has_trace_data() const {
  bool value = (_impl_._has_bits_[0] & 0x00000010u) != 0;
  return value;
}
inline bool MqMsg::has_trace_data() const {
  return _internal_has_trace_data();
}
inline void MqMsg::clear_trace_data() {
  _impl_.trace_data_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000010u;
}
inline const std::string& MqMsg::trace_data() const {
  // @@protoc_insertion_point(field_get:jukey.prot.MqMsg.trace_data)
  return _internal_trace_data();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void MqMsg::set_trace_data(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000010u;
 _impl_.trace_data_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:jukey.prot.MqMsg.trace_data)
}
inline std::string* MqMsg::mutable_trace_data() {
  std::string* _s = _internal_mutable_trace_data();
  // @@protoc_insertion_point(field_mutable:jukey.prot.MqMsg.trace_data)
  return _s;
}
inline const std::string& MqMsg::_internal_trace_data() const {
  return _impl_.trace_data_.Get();
}
inline void MqMsg::_internal_set_trace_data(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000010u;
  _impl_.trace_data_.Set(value, GetArenaForAllocation());
}
inline std::string* MqMsg::_internal_mutable_trace_data() {
  _impl_._has_bits_[0] |= 0x00000010u;
  return _impl_.trace_data_.Mutable(GetArenaForAllocation());
}
inline std::string* MqMsg::release_trace_data() {
  // @@protoc_insertion_point(field_release:jukey.prot.MqMsg.trace_data)
  if (!_internal_has_trace_data()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000010u;
  auto* p = _impl_.trace_data_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.trace_data_.IsDefault()) {
    _impl_.trace_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void MqMsg::set_allocated_trace_data(std::string* trace_data) {
  if (trace_data != nullptr) {
    _impl_._has_bits_[0] |= 0x00000010u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000010u;
  }
  _impl_.trace_data_.SetAllocated(trace_data, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.trace_data_.IsDefault()) {
    _impl_.trace_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:jukey.prot.MqMsg.trace_data)
}

// optional string extend_data = 7;
inline bool MqMsg::_internal_has_extend_data() const {
  bool value = (_impl_._has_bits_[0] & 0x00000020u) != 0;
  return value;
}
inline bool MqMsg::has_extend_data() const {
  return _internal_has_extend_data();
}
inline void MqMsg::clear_extend_data() {
  _impl_.extend_data_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000020u;
}
inline const std::string& MqMsg::extend_data() const {
  // @@protoc_insertion_point(field_get:jukey.prot.MqMsg.extend_data)
  return _internal_extend_data();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void MqMsg::set_extend_data(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000020u;
 _impl_.extend_data_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:jukey.prot.MqMsg.extend_data)
}
inline std::string* MqMsg::mutable_extend_data() {
  std::string* _s = _internal_mutable_extend_data();
  // @@protoc_insertion_point(field_mutable:jukey.prot.MqMsg.extend_data)
  return _s;
}
inline const std::string& MqMsg::_internal_extend_data() const {
  return _impl_.extend_data_.Get();
}
inline void MqMsg::_internal_set_extend_data(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000020u;
  _impl_.extend_data_.Set(value, GetArenaForAllocation());
}
inline std::string* MqMsg::_internal_mutable_extend_data() {
  _impl_._has_bits_[0] |= 0x00000020u;
  return _impl_.extend_data_.Mutable(GetArenaForAllocation());
}
inline std::string* MqMsg::release_extend_data() {
  // @@protoc_insertion_point(field_release:jukey.prot.MqMsg.extend_data)
  if (!_internal_has_extend_data()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000020u;
  auto* p = _impl_.extend_data_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.extend_data_.IsDefault()) {
    _impl_.extend_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void MqMsg::set_allocated_extend_data(std::string* extend_data) {
  if (extend_data != nullptr) {
    _impl_._has_bits_[0] |= 0x00000020u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000020u;
  }
  _impl_.extend_data_.SetAllocated(extend_data, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.extend_data_.IsDefault()) {
    _impl_.extend_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:jukey.prot.MqMsg.extend_data)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace prot
}  // namespace jukey

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_mq_2eproto
