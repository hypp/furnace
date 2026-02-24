// src/engine/platform/i8253.cpp
#include "i8253.h"
#include "../engine.h"
#include <math.h>

#define CHIP_DIVIDER 1


DivPlatform8253::DivPlatform8253():
      DivDispatch(),
      oscBuf(NULL),
      isMuted(false) {

}

void DivPlatform8253::acquire(short** buf, size_t len) {
  oscBuf->begin(len);
  for (size_t i = 0; i < len; i++) {
    // Scale down to match SN76489
    short out = (on && !isMuted) ? (chip.tick(rate)/4.0) : 0;
    buf[0][i] = out;
    oscBuf->putSample(i, out);
  }
  oscBuf->end(len);
}

int DivPlatform8253::dispatch(DivCommand c) {
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON: {
      DivInstrument* ins=parent->getIns(chan[0].ins,DIV_INS_BEEPER);
      chan[0].macroInit(ins);
      if (c.value!=DIV_NOTE_NULL) {
        chan[0].baseFreq=NOTE_PERIODIC(c.value);
        chan[0].freqChanged=true;
        chan[0].note=c.value;
      }
      chan[0].active=true;
      chan[0].keyOn=true;
      if (!parent->song.compatFlags.brokenOutVol && !chan[0].std.vol.will) {
        chan[0].outVol=chan[0].vol;
      }
      break;
    }
    case DIV_CMD_NOTE_OFF_ENV:
    case DIV_CMD_NOTE_OFF:
      chan[0].active=false;
      chan[0].keyOff=true;
      chan[0].macroInit(NULL);
      on=false;
      break;
    case DIV_CMD_ENV_RELEASE:
      chan[0].std.release();
      break;
    case DIV_CMD_INSTRUMENT:
      if (chan[0].ins!=c.value || c.value2==1) {
        chan[0].ins=c.value;
        chan[0].insChanged=true;
      }
      break;
    case DIV_CMD_PITCH:
      chan[0].pitch=c.value;
      chan[0].freqChanged=true;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=NOTE_PERIODIC(c.value2);
      bool return2=false;
      if (destFreq>chan[0].baseFreq) {
        chan[0].baseFreq+=c.value;
        if (chan[0].baseFreq>=destFreq) {
          chan[0].baseFreq=destFreq;
          return2=true;
        }
      } else {
        chan[0].baseFreq-=c.value;
        if (chan[0].baseFreq<=destFreq) {
          chan[0].baseFreq=destFreq;
          return2=true;
        }
      }
      chan[0].freqChanged=true;
      if (return2) return 2;
      break;
    }
    case DIV_CMD_LEGATO:
      chan[0].baseFreq=NOTE_PERIODIC(c.value+((chan[0].std.arp.will && !chan[0].std.arp.mode)?(chan[0].std.arp.val):(0)));
      chan[0].freqChanged=true;
      chan[0].note=c.value;
      break;
    case DIV_CMD_GET_VOLMAX:
      return 1;
    case DIV_CMD_MACRO_OFF:
      chan[0].std.mask(c.value,true);
      break;
    case DIV_CMD_MACRO_ON:
      chan[0].std.mask(c.value,false);
      break;
    case DIV_CMD_MACRO_RESTART:
      chan[0].std.restart(c.value);
      break;
    default:
      break;
  }
  return 1;
}

void DivPlatform8253::tick(bool sysTick) {
  chan[0].std.next();

  if (chan[0].std.vol.had) {
    chan[0].outVol=chan[0].std.vol.val&chan[0].vol;
    on=(chan[0].active && chan[0].outVol>0);
  }

  if (chan[0].std.arp.had) {
    if (!chan[0].std.arp.mode) {
      chan[0].baseFreq=NOTE_PERIODIC(chan[0].note+chan[0].std.arp.val);
    } else {
      chan[0].baseFreq=NOTE_PERIODIC(chan[0].std.arp.val);
    }
    chan[0].freqChanged=true;
  }

  if (chan[0].std.pitch.had) {
    chan[0].pitch2=chan[0].std.pitch.val;
    chan[0].freqChanged=true;
  }

  if (chan[0].freqChanged || chan[0].keyOn || chan[0].keyOff) {
    if (chan[0].keyOn) on=true;
    if (chan[0].keyOff) {
      on=false;
      if (dumpWrites) {
        // We use 0 for note off
        addWrite(0x00, 0x0000);
      }
    }

    chan[0].freq=parent->calcFreq(chan[0].baseFreq,chan[0].pitch,chan[0].fixedArp?chan[0].baseNoteOverride:chan[0].arpOff,chan[0].fixedArp,true,0,chan[0].pitch2,chipClock,CHIP_DIVIDER);
    if (chan[0].freq<1) chan[0].freq=1;
    if (chan[0].freq>65535) chan[0].freq=65535;
    chip.setFreq((uint16_t)chan[0].freq);

    if (!chan[0].keyOff && chan[0].active) {
      if (dumpWrites) {
        addWrite(0x00, chan[0].freq);
      }
    }

    chan[0].freqChanged=false;
    chan[0].keyOn=false;
    chan[0].keyOff=false;
  }
}

void* DivPlatform8253::getChanState(int c) {
  return &chan[c];
}

DivMacroInt* DivPlatform8253::getChanMacroInt(int c) {
  return &chan[c].std;
}

DivDispatchOscBuffer* DivPlatform8253::getOscBuffer(int c) {
  return oscBuf;
}

unsigned char* DivPlatform8253::getRegisterPool() {
  return NULL;
}

int DivPlatform8253::getRegisterPoolSize() {
  return 0;
}

const char** DivPlatform8253::getRegisterPoolNames() {
  return NULL;
}

void DivPlatform8253::reset() {
  on=false;
  chan[0]=Channel();
  chan[0].std.setEngine(parent);
  chip.reset();

  if (dumpWrites) {
    addWrite(0xffffffff,0);
  }
}

void DivPlatform8253::forceIns() {
  chan[0].insChanged=true;
  chan[0].freqChanged=true;
}

void DivPlatform8253::muteChannel(int ch, bool mute) {
  isMuted=mute;
}

int DivPlatform8253::getOutputCount() {
  return 1;
}

void DivPlatform8253::setFlags(const DivConfig& flags) {
  chipClock=1108400;
}

void DivPlatform8253::notifyInsDeletion(void* ins) {
  chan[0].std.notifyInsDeletion((DivInstrument*)ins);
}

int DivPlatform8253::init(DivEngine* p, int channels, int suggestedRate, const DivConfig& flags) {
  parent=p;
  dumpWrites=false;
  skipRegisterWrites=false;
  oscBuf=new DivDispatchOscBuffer;
  isMuted=false;
  setFlags(flags);
  rate=suggestedRate;
  chip.chipClock=chipClock;
  reset();
  return 1;
}

void DivPlatform8253::quit() {
  delete oscBuf;
}

bool DivPlatform8253::keyOffAffectsArp(int ch) {
  return true;
}

void DivPlatform8253::notifyPlaybackStop() {
  on=false;
  chan[0].active=false;
}

void DivPlatform8253::toggleRegisterDump(bool enable) {
  DivDispatch::toggleRegisterDump(enable);
  if (enable) {
    getRegisterWrites().clear();
  }
}
