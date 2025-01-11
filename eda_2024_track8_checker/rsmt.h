////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, Iowa State University All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <climits>
#include <vector>
#include <string>
#include <list>

// Rectilinear Steiner Min Tree
/*****************************/
/*  User-Defined Parameters  */
/*****************************/
#define FLUTE_ROUTING 1               // 1 to construct routing, 0 to estimate WL only
#define FLUTE_LOCAL_REFINEMENT 1      // Suggestion: Set to 1 if ACCURACY >= 5
#define FLUTE_REMOVE_DUPLICATE_PIN 0  // Remove dup. pin for flute_wl() & flute()

#define FLUTE_POWVFILE "../FLUTE_LIB/POWV9.dat"  // LUT for POWV (Wirelength Vector)
#define FLUTE_POSTFILE "../FLUTE_LIB/POST9.dat"  // LUT for POST (Steiner Tree)

#define FLUTE_D 9                   // LUT is used for d <= FLUTE_D, FLUTE_D <= 9

// Use flute LUT file reader.
#define LUT_FILE 1
// Init LUTs from base64 encoded string variables.
#define LUT_VAR 2
// Init LUTs from base64 encoded string variables
// and check against LUTs from file reader.
#define LUT_VAR_CHECK 3

// Set this to LUT_FILE, LUT_VAR, or LUT_VAR_CHECK.
#define LUT_SOURCE LUT_FILE
//#define LUT_SOURCE LUT_VAR
//#define LUT_SOURCE LUT_VAR_CHECK

typedef int DTYPE;

struct Branch{
  DTYPE x, y;  // starting point of the branch
  int n;       // index of neighbor
  Branch () {
    x = INT_MAX;
    y = INT_MAX;
    n = INT_MAX;
  };
};

struct Tree {
  int deg;         // degree
  DTYPE length;    // total wirelength
  Branch *branch;  // array of tree branches

  Tree() {
    deg = 0;
    length = 0;
    branch = nullptr;
  };
};

struct point {
  DTYPE x, y;
  int o;
};

struct csoln {
  unsigned char parent;
  unsigned char seg[11];        // Add: 0..i, Sub: j..10; seg[i+1]=seg[j-1]=0
  unsigned char rowcol[FLUTE_D - 2];  // row = rowcol[]/16, col = rowcol[]%16,
  unsigned char neighbor[2 * FLUTE_D - 2];
};

typedef struct csoln ***LUT_TYPE;
typedef int **NUMSOLN_TYPE;
// struct csoln *LUT[FLUTE_D + 1][MGROUP];  // storing 4 .. FLUTE_D
// int numsoln[FLUTE_D + 1][MGROUP];

template <class T> inline T ADIFF(T x, T y) {
  if (x > y) {
    return (x - y);
  } else {
    return (y - x);
  }
}

class RecSteinerMinTree {

  std::string goldenPost9_;  // to be moved to device database
  std::string goldenPowv9_;

  std::string post9_;
  std::string powv9_;

  std::list<std::string> treeArr_;     // original lib data
  std::list<std::string> vectorArr_;

  int accuracy_;    // acc =3 by default

  int lutInitialDegree_;
  int lutValidDegree_;

  LUT_TYPE SteinerLut_;    // *LUT[FLUTE_D + 1][MGROUP];  storing 4 .. FLUTE_D
  NUMSOLN_TYPE numSoln_; // numsoln[FLUTE_D + 1][MGROUP];

 public:
  RecSteinerMinTree();
  ~RecSteinerMinTree();

  // member access
  std::list<std::string>& treeArr(void) { return treeArr_; }
  std::list<std::string>& vectorArr(void) { return vectorArr_; }
  void addTreeData(const std::string& str) { treeArr_.push_back(str); }
  void addVectorData(const std::string& str) { vectorArr_.push_back(str); }

// User-Callable Functions
  int accuracy() const { return accuracy_; }
  void setAccuracy(const int acc) { accuracy_ = acc; }

  DTYPE fltWireLength(std::vector<DTYPE>& x, std::vector<DTYPE>& y);
  Tree fltTree(std::vector<DTYPE>& x, std::vector<DTYPE>& y);
  DTYPE wirelength(Tree t);
  void printtree(Tree t);
  void plottree(Tree t);
  void write_svg(Tree t, const char *filename);
  void free_tree(Tree t);

private:

// LUT lib
  void readLUT();
  void deleteLUT();
  unsigned char charNum(unsigned char c);
  bool is_base64(unsigned char c);
  std::string base64_decode(std::string const& encoded_string);
  void readLUTfiles();
  void makeLUT(LUT_TYPE &LUT, NUMSOLN_TYPE &numsoln);
  void deleteLUT(LUT_TYPE &LUT, NUMSOLN_TYPE &numsoln);
  void initLUT(int to_d, LUT_TYPE LUT, NUMSOLN_TYPE numsoln);
  void ensureLUT(int d);
  void checkLUT(LUT_TYPE LUT1, NUMSOLN_TYPE numsoln1, LUT_TYPE LUT2,
      NUMSOLN_TYPE numsoln2);

// Tree
  Tree dmergetree(Tree t1, Tree t2);
  Tree hmergetree(Tree t1, Tree t2, int s[]);
  Tree vmergetree(Tree t1, Tree t2);
  void local_refinement(int deg, Tree *tp, int p);

// util.
  static int orderx(const void *a, const void *b);
  static int ordery(const void *a, const void *b);

// Other useful functions
  DTYPE flutes_wl(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);
  DTYPE flutes_wl_ALLD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);
  DTYPE flutes_wl_LMD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);
  DTYPE flutes_wl_LD(int d, DTYPE xs[], DTYPE ys[], int s[]);
  DTYPE flutes_wl_MD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);
  DTYPE flutes_wl_RDP(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);

  Tree flutes(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);
  Tree flutes_ALLD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc); // all degress nets
  Tree flutes_RDP(int d, DTYPE xs[], DTYPE ys[], int s[], int acc); // FLUTE_REMOVE_DUPLICATE_PIN
  Tree flutes_LMD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc); // same as ALLD
  Tree flutes_LD(int d, DTYPE xs[], DTYPE ys[], int s[]);      // Low degree net
  Tree flutes_MD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc); // mid degree net
  Tree flutes_HD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);

}; // end class RecSteinerMinTree
