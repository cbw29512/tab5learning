#pragma once
#include <Preferences.h>
#include "math_checkpoint.h"

enum class MathLoad { Empty, Restored, Error };
class MathStorage {
  Preferences prefs;
  bool ready=false;
 public:
  MathLoad begin(MathPractice& state) {
    ready=prefs.begin("pocketlearn",false);
    if(!ready) return MathLoad::Error;
    if(!prefs.isKey("mathv1")) return MathLoad::Empty;
    MathCheckpoint bytes{};
    if(prefs.getBytesLength("mathv1")!=bytes.size() ||
       prefs.getBytes("mathv1",bytes.data(),bytes.size())!=bytes.size() ||
       !math_decode(bytes,state)) {
      ready=false;
      return MathLoad::Error;
    }
    return MathLoad::Restored;
  }
  bool save(const MathPractice& state) {
    MathCheckpoint bytes{}, verify{};
    if(!ready || !math_encode(state,bytes)) return false;
    if(prefs.putBytes("mathv1",bytes.data(),bytes.size())!=bytes.size() ||
       prefs.getBytes("mathv1",verify.data(),verify.size())!=verify.size() ||
       verify!=bytes) {
      // An ambiguous commit must be reconciled on restart, never retried blindly.
      ready=false;
      return false;
    }
    return true;
  }
};
