#!/bin/bash
g++ mbus-inspect-frame.cpp -I../ -lmbus -L../mbus/.libs/ -static -o mbus-inspect-frame
