/*************************************************
* ElGamal Operations Source File                 *
* (C) 1999-2007 Jack Lloyd                       *
*************************************************/

#include <botan/eng_def.h>
#include <botan/pow_mod.h>
#include <botan/numthry.h>
#include <botan/reducer.h>

namespace Botan {

namespace {

/*************************************************
* Default ElGamal Operation                      *
*************************************************/
class Default_ELG_Op : public ELG_Operation
   {
   public:
      SecureVector<byte> encrypt(const byte[], u32bit, const BigInt&) const;
      BigInt decrypt(const BigInt&, const BigInt&) const;

      ELG_Operation* clone() const { return new Default_ELG_Op(*this); }

      Default_ELG_Op(const DL_Group&, const BigInt&, const BigInt&);
   private:
      const BigInt p;
      Fixed_Base_Power_Mod powermod_g_p, powermod_y_p;
      Fixed_Exponent_Power_Mod powermod_x_p;
      Modular_Reducer mod_p;
   };

/*************************************************
* Default_ELG_Op Constructor                     *
*************************************************/
Default_ELG_Op::Default_ELG_Op(const DL_Group& group, const BigInt& y,
                               const BigInt& x) : p(group.get_p())
   {
   powermod_g_p = Fixed_Base_Power_Mod(group.get_g(), p);
   powermod_y_p = Fixed_Base_Power_Mod(y, p);
   mod_p = Modular_Reducer(p);

   if(x != 0)
      powermod_x_p = Fixed_Exponent_Power_Mod(x, p);
   }

/*************************************************
* Default ElGamal Encrypt Operation              *
*************************************************/
SecureVector<byte> Default_ELG_Op::encrypt(const byte in[], u32bit length,
                                           const BigInt& k) const
   {
   BigInt m(in, length);
   if(m >= p)
      throw Invalid_Argument("Default_ELG_Op::encrypt: Input is too large");

   BigInt a = powermod_g_p(k);
   BigInt b = mod_p.multiply(m, powermod_y_p(k));

   SecureVector<byte> output(2*p.bytes());
   a.binary_encode(output + (p.bytes() - a.bytes()));
   b.binary_encode(output + output.size() / 2 + (p.bytes() - b.bytes()));
   return output;
   }

/*************************************************
* Default ElGamal Decrypt Operation              *
*************************************************/
BigInt Default_ELG_Op::decrypt(const BigInt& a, const BigInt& b) const
   {
   if(a >= p || b >= p)
      throw Invalid_Argument("Default_ELG_Op: Invalid message");

   return mod_p.multiply(b, inverse_mod(powermod_x_p(a), p));
   }

}

/*************************************************
* Acquire an ElGamal op                          *
*************************************************/
ELG_Operation* Default_Engine::elg_op(const DL_Group& group, const BigInt& y,
                                      const BigInt& x) const
   {
   return new Default_ELG_Op(group, y, x);
   }

}
