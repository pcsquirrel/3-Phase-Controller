#include "TripleBuffer.cpp"
#include "Config.h"

template class TripleBuffer<Config::i2cBufferIncomingSize, true>;
template class TripleBuffer<Config::i2cBufferOutgoingSize, false>;
