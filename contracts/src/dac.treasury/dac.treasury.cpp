#include "../include/dac/dac.treasury.hpp"



namespace eosio {


void treasury::handle_transfer( name from, name to, asset quantity, std::string memo ) {
      // just in case
  if (from == _self ) return;
    
  // EOSBet hack
  eosio_assert( to == _self, "Buy Error: Transfer must be direct to contract." ); 
  eosio_assert( quantity.symbol == symbol("EOS", 4), "you must transfer in EOS." );
  eosio_assert( quantity.is_valid(), "quantity is not valid." );
  
  //count the third part
  symbol eos_symbol = symbol("EOS", 4);
  asset part = asset( 0, eos_symbol );
  part.amount = floor( quantity.amount / 3 ); // rounding down
  
  // requires @eosio@code repmission for _self
  action( permission_level{_self, name("active")},
    name("eosio.token"), name("transfer"),
    std::make_tuple(_self, name("btce111v1112"), part,
        std::string("treasury: fund fill")))
  .send();
  
  // todo: get this guy from a table?
  action( permission_level{_self, name("active")},
    name("eosio.token"), name("transfer"),
    std::make_tuple(_self, name("btce111v1113"), part,
        std::string("treasury: dapp price pool fill")))
  .send();
  
  action( permission_level{_self, name("active")},
    name("eosio.token"), name("transfer"),
    std::make_tuple(_self, name("btce111v1121"), part,
        std::string("treasury: the first dapp reward!")))
  .send();
}





extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
      if (code == name("eosio.token").value && action == name("transfer").value) {
          execute_action(name(receiver), name(code), &treasury::handle_transfer);
      }
    }
}



} // namespace eosio
