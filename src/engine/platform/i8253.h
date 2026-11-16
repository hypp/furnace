
#pragma once
#include "../dispatch.h"
#include "../macroInt.h"
#include "sound/i8253chip.h"

class DivPlatform8253: public DivDispatch {

  struct Channel: public SharedChannel {
    Channel(bool linear = true):
      SharedChannel(1,linear) {
      }
  };

  Channel chan[1];
  DivPitchTable pitchTable;
  DivDispatchOscBuffer* oscBuf;
  bool isMuted;
  bool on;
  i8253_device chip;

  public:
    void acquire(short** buf, size_t len);
    int dispatch(DivCommand c);
    SharedChannel* getChanState(int chan);
    DivMacroInt* getChanMacroInt(int ch);
    DivDispatchOscBuffer* getOscBuffer(int chan);
    unsigned char* getRegisterPool();
    int getRegisterPoolSize();
    void reset();
    void forceIns();
    void tick(bool sysTick=true);
    void muteChannel(int ch, bool mute);
    int getOutputCount();
    void setFlags(const DivConfig& flags);
    void notifyInsDeletion(void* ins);
    const char** getRegisterPoolNames();
    int init(DivEngine* parent, int channels, int suggestedRate, const DivConfig& flags);
    void quit();
    bool keyOffAffectsArp(int ch);
    void notifyPlaybackStop();
    void toggleRegisterDump(bool enable);

    DivPlatform8253();

};