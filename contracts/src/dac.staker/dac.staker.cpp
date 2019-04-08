#include "../../include/dac/dac.staker.hpp"


namespace eosio {

using std::string;

void staker::handle_transfer( name from, name to, asset quantity, std::string memo ) {
    if (from == _self) return;
    // EOSBet hack
    eosio_assert(to == _self, "transfer must be direct to contract.");
    // Check the buy code is valid
    const string transfer_code = memo.substr(0, 7);
    
    eosio_assert(transfer_code == "staking" || transfer_code == "", "malformed memo string");
    if (transfer_code == "") { return; }
    
    if (transfer_code == "staking") {
      eosio_assert(quantity.symbol == symbol("EOS", 4), "you must transfer EOS tokens for staking");
      eosio_assert( quantity.is_valid(), "quantity is not valid." );
      handle_staking( quantity );
    }
    
   
}

void staker::handle_staking( asset quantity ) {
  dapps _dapps( VOTER_CONTRACT, VOTER_CONTRACT.value);
  
  // todo: add total_staked field to the voting contract
  
  asset total_asset = _get_voter_total_votes();
  int total = total_asset.amount;
  eosio_assert( total > 0,
    string("total votes is 0. Cannot stake: " + std::to_string(total)).c_str());
  
  for( const auto& dapp : _dapps ) {
    
    if ( is_account(dapp.dappname) && dapp.votes.amount > 0 ) {
      // in propotrion on the number of votes
      int part_amount = floor( quantity.amount * dapp.votes.amount / (total*2) );
      asset part = asset( part_amount, symbol("EOS", 4) );
      eosio_assert( part.amount > 0, 
        string("part: " + dapp.dappname.to_string() + " : " + std::to_string(part.amount)).c_str());
    
      action( permission_level{_self, name("active")},
        name("eosio"), name("delegatebw"),
        std::make_tuple( _self, name(dapp.dappname), part, part, false) )
      .send();
    }
    
  }
  
}

void staker::distribute() {
  require_auth( _self );
  
  // getting the account balance
  accounts acc( name("eosio.token"), _self.value ); 
  symbol eos_sym = symbol("EOS", 4);
  auto ac = acc.get(eos_sym.code().raw()); 
  // to exclude overdrawn balance
  ac.balance.amount -= 1;
  
  // eosio_assert(false, string("balance: " + std::to_string(ac.balance.amount)).c_str() );
  
  eosio_assert( ac.balance.symbol == symbol("EOS", 4), "you must have EOS tokens for staking");
  eosio_assert( ac.balance.is_valid(), "quantity is not valid." );
  handle_staking( ac.balance );
}



asset staker::_get_voter_total_votes() {
  settings config_voter{VOTER_CONTRACT, VOTER_CONTRACT.value };
  auto itr_config = config_voter.find( name("settings").value );
  
  eosio_assert( itr_config != config_voter.end(), 
    string("the settings does not exists on the voter contract: " + VOTER_CONTRACT.to_string()).c_str() );
  
  return itr_config->totalvotes;
}




extern "C" {
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    
    // todo: important:: here should be a name of the account with the MG token
    // todo: put it in the Singleton
    if (code == name("eosio.token").value && action == name("transfer").value) {
      execute_action(name(receiver), name(code), &staker::handle_transfer);
    } 
    else if (code == receiver && action == name("distribute").value) {
      execute_action(name(receiver), name(code), &staker::distribute);
    }
    
    else if (code == receiver && action == name("diserbute").value) {
      execute_action(name(receiver), name(code), &staker::distribute);
    }
  }
  
}


} // eosio namespace


