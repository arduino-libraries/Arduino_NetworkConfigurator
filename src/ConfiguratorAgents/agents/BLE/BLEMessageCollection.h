/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include <memory>

template <typename T, int N>
class BLEMessageCollection
{
  public:
    bool addMessage(T &msg){
      if(_numMessages < N){
        _messages[_head] = msg;
        _head = nextMessageIdx(_head);
        _numMessages++;

        return true;
      }
      return false;
    }

    uint8_t numMessages() {return _numMessages;};
    void clear(){
      for(uint8_t i = 0; i < _numMessages; i++){
        _messages[i].reset();
      }
      _numMessages = 0;
      _head = 0;
      _tail = 0;
    }

    T* nextMessage(int startPos = -1){
      if(startPos != -1){
        _iteratorIdx = startPos;
      }
      
      if(_numMessages == 0){
        return nullptr;
      }
      
      if(_iteratorIdx > N){
        _iteratorIdx = 0;
        return nullptr;
      }

      T *msg = &_messages[_iteratorIdx];
      _iteratorIdx++;
      return msg;
    }

    T* frontMessage() {
      if(_numMessages == 0){
        return nullptr;
      }
      T *msg = &_messages[_tail];
      return msg;
    }
    void popMessage(){
      if(_numMessages == 0){
        return;
      }
      _messages[_tail].reset();
      _numMessages--;
      _tail = nextMessageIdx(_tail);
      return;
    }

  private:
    int nextMessageIdx(int elem){ 
      elem++;
      return elem%N;
    }
    T _messages[N];
    uint8_t _numMessages = 0;
    uint8_t _head = 0;
    uint8_t _tail = 0;
    uint8_t _iteratorIdx = 0;
};
