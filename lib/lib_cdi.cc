#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

std::map<unsigned, unsigned> instr_map;

const char *mapCodeToName(unsigned Op) {
  if (Op == 1) {
    return "ret";
  } else if (Op == 2) {
    return "br";
  } else if (Op == 3) {
    return "switch";
  } else if (Op == 4) {
    return "indirectbr";
  } else if (Op == 5) {
    return "invoke";
  } else if (Op == 6) {
    return "resume";
  } else if (Op == 7) {
    return "unreachable";
  } else if (Op == 8) {
    return "cleanupret";
  } else if (Op == 9) {
    return "catchret";
  } else if (Op == 10) {
    return "catchswitch";
  } else if (Op == 11) {
    return "add";
  } else if (Op == 12) {
    return "fadd";
  } else if (Op == 13) {
    return "sub";
  } else if (Op == 14) {
    return "fsub";
  } else if (Op == 15) {
    return "mul";
  } else if (Op == 16) {
    return "fmul";
  } else if (Op == 17) {
    return "udiv";
  } else if (Op == 18) {
    return "sdiv";
  } else if (Op == 19) {
    return "fdiv";
  } else if (Op == 20) {
    return "urem";
  } else if (Op == 21) {
    return "srem";
  } else if (Op == 22) {
    return "frem";
  } else if (Op == 23) {
    return "shl";
  } else if (Op == 24) {
    return "lshr";
  } else if (Op == 25) {
    return "ashr";
  } else if (Op == 26) {
    return "and";
  } else if (Op == 27) {
    return "or";
  } else if (Op == 28) {
    return "xor";
  } else if (Op == 29) {
    return "alloca";
  } else if (Op == 30) {
    return "load";
  } else if (Op == 31) {
    return "store";
  } else if (Op == 32) {
    return "getelementptr";
  } else if (Op == 33) {
    return "fence";
  } else if (Op == 34) {
    return "cmpxchg";
  } else if (Op == 35) {
    return "atomicrmw";
  } else if (Op == 36) {
    return "trunc";
  } else if (Op == 37) {
    return "zext";
  } else if (Op == 38) {
    return "sext";
  } else if (Op == 39) {
    return "fptoui";
  } else if (Op == 40) {
    return "fptosi";
  } else if (Op == 41) {
    return "uitofp";
  } else if (Op == 42) {
    return "sitofp";
  } else if (Op == 43) {
    return "fptrunc";
  } else if (Op == 44) {
    return "fpext";
  } else if (Op == 45) {
    return "ptrtoint";
  } else if (Op == 46) {
    return "inttoptr";
  } else if (Op == 47) {
    return "bitcast";
  } else if (Op == 48) {
    return "addrspacecast";
  } else if (Op == 49) {
    return "cleanuppad";
  } else if (Op == 50) {
    return "catchpad";
  } else if (Op == 51) {
    return "icmp";
  } else if (Op == 52) {
    return "fcmp";
  } else if (Op == 53) {
    return "phi";
  } else if (Op == 54) {
    return "call";
  } else if (Op == 55) {
    return "select";
  } else if (Op == 56) {
    return "<Invalid operator>";
  } else if (Op == 57) {
    return "<Invalid operator>";
  } else if (Op == 58) {
    return "va_arg";
  } else if (Op == 59) {
    return "extractelement";
  } else if (Op == 60) {
    return "insertelement";
  } else if (Op == 61) {
    return "shufflevector";
  } else if (Op == 62) {
    return "extractvalue";
  } else if (Op == 63) {
    return "insertvalue";
  } else if (Op == 64) {
    return "landingpad";
  }

  return "<Invalid operator>";
}

// Update instruction information.
// Add <num> pairs of <opcode, count> info instruction map.
extern "C" __attribute__((visibility("default")))
void updateInstrInfo(unsigned num, uint32_t* keys, uint32_t* values) {
  uint32_t key;
  uint32_t value;

  for (int i = 0; i < num; ++i) {
    key = keys[i];
    value = values[i];
    if (instr_map.count(key) == 0) {
      instr_map.insert(std::pair<uint32_t, uint32_t>(key, value));
    } else {
      instr_map[key] = instr_map[key] + value;
    }
  }
}

extern "C" __attribute__((visibility("default")))
void printOutInstrInfo() {
  for (std::map<uint32_t, uint32_t>::iterator it = instr_map.begin();
       it != instr_map.end(); ++it) {
    fprintf(stderr, "%s\t%u\n", mapCodeToName(it->first), it->second);
  }

  instr_map.clear();
}
