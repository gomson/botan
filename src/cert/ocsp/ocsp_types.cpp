/*
* OCSP subtypes
* (C) 2012 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/ocsp_types.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>
#include <botan/x509_ext.h>
#include <botan/lookup.h>
#include <botan/hash.h>
#include <botan/oids.h>
#include <memory>

namespace Botan {

namespace OCSP {

CertID::CertID(const X509_Certificate& issuer,
               const X509_Certificate& subject)
   {
   /*
   In practice it seems some responders, including, notably,
   ocsp.verisign.com, will reject anything but SHA-1 here
   */
   std::unique_ptr<HashFunction> hash(get_hash("SHA-160"));

   m_hash_id = AlgorithmIdentifier(hash->name(), AlgorithmIdentifier::USE_NULL_PARAM);
   m_issuer_key_hash = unlock(hash->process(extract_key_bitstr(issuer)));
   m_issuer_dn_hash = unlock(hash->process(subject.raw_issuer_dn()));
   m_subject_serial = BigInt::decode(subject.serial_number());
   }

std::vector<byte> CertID::extract_key_bitstr(const X509_Certificate& cert) const
   {
   const auto key_bits = cert.subject_public_key_bits();

   AlgorithmIdentifier public_key_algid;
   std::vector<byte> public_key_bitstr;

   BER_Decoder(key_bits)
      .decode(public_key_algid)
      .decode(public_key_bitstr, BIT_STRING);

   return public_key_bitstr;
   }

bool CertID::is_id_for(const X509_Certificate& issuer,
                       const X509_Certificate& subject) const
   {
   try
      {
      if(BigInt::decode(subject.serial_number()) != m_subject_serial)
         return false;

      std::unique_ptr<HashFunction> hash(get_hash(OIDS::lookup(m_hash_id.oid)));

      if(m_issuer_dn_hash != unlock(hash->process(subject.raw_issuer_dn())))
         return false;

      if(m_issuer_key_hash != unlock(hash->process(extract_key_bitstr(issuer))))
         return false;
      }
   catch(...)
      {
      return false;
      }

   return true;
   }

void CertID::encode_into(class DER_Encoder& to) const
   {
   to.start_cons(SEQUENCE)
      .encode(m_hash_id)
      .encode(m_issuer_dn_hash, OCTET_STRING)
      .encode(m_issuer_key_hash, OCTET_STRING)
      .encode(m_subject_serial)
      .end_cons();
   }

void CertID::decode_from(class BER_Decoder& from)
   {
   from.start_cons(SEQUENCE)
      .decode(m_hash_id)
      .decode(m_issuer_dn_hash, OCTET_STRING)
      .decode(m_issuer_key_hash, OCTET_STRING)
      .decode(m_subject_serial)
      .end_cons();

   }

bool SingleResponse::affirmative_response_for(
   const X509_Certificate& issuer,
   const X509_Certificate& subject) const
   {
   if(!m_good_status)
      return false;

   if(!m_certid.is_id_for(issuer, subject))
      return false;

   X509_Time current_time(std::chrono::system_clock::now());

   if(m_thisupdate > current_time)
      return false; // not yet valid?

   if(m_nextupdate.time_is_set() && current_time > m_nextupdate)
      return false; // expired, probably replayed

   return true;
   }

void SingleResponse::encode_into(class DER_Encoder&) const
   {
   throw std::runtime_error("Not implemented (SingleResponse::encode_into)");
   }

void SingleResponse::decode_from(class BER_Decoder& from)
   {
   BER_Object cert_status;
   Extensions extensions;

   from.start_cons(SEQUENCE)
      .decode(m_certid)
      .get_next(cert_status)
      .decode(m_thisupdate)
      .decode_optional(m_nextupdate, ASN1_Tag(0),
                       ASN1_Tag(CONTEXT_SPECIFIC | CONSTRUCTED))
      .decode_optional(extensions,
                       ASN1_Tag(1),
                       ASN1_Tag(CONTEXT_SPECIFIC | CONSTRUCTED))
      .end_cons();

   m_good_status = (cert_status.type_tag == 0);
   }

}

}
