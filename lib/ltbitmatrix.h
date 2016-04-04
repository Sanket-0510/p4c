#ifndef P4C_LIB_LTBITMATRIX_H_
#define P4C_LIB_LTBITMATRIX_H_

#include "bitvec.h"

/* A lower-triangular bit matrix, held in a bit vector */
class LTBitMatrix : private bitvec {
 public:
    bitref operator()(unsigned r, unsigned c) {
        return r >= c ? bitvec::operator[]((r*r+r)/2 + c) : end(); }
    bool operator()(unsigned r, unsigned c) const {
        return  r >= c ? bitvec::operator[]((r*r+r)/2 + c) : false; }
    unsigned size() const {
        if (empty()) return 0;
        unsigned m = *max();
        unsigned r = 1;
        while ((r*r+r)/2 <= m) r++;
        return r; }
    using bitvec::clear;
    using bitvec::empty;
    using bitvec::operator bool;

 private:
    template<class T> class rowref {
        friend class LTBitMatrix;
        T               &self;
        unsigned        row;
        rowref(T &s, unsigned r) : self(s), row(r) {}

     public:
        rowref(const rowref &) = default;
        rowref(rowref &&) = default;
        explicit operator bool() const {
            if (row < bits_per_unit)
                return self.getrange((row*row+row)/2, row+1) != 0;
            else
                return self.getslice((row*row+row)/2, row+1) ? true : false; }
        operator bitvec() const { return self.getslice((row*row+row)/2, row+1); }
    };
    class nonconst_rowref : public rowref<LTBitMatrix> {
     public:
        friend class LTBitMatrix;
        using rowref<LTBitMatrix>::rowref;
        void operator|=(bitvec a) const { a.clrrange(row+1, ~0); self |= a << (row*row+row)/2; }
        bitref operator[](unsigned col) const { return self(row, col); }
    };
    class const_rowref : public rowref<const LTBitMatrix> {
     public:
        friend class LTBitMatrix;
        using rowref<const LTBitMatrix>::rowref;
        bool operator[](unsigned col) const { return self(row, col); }
    };

 public:
    nonconst_rowref operator[](unsigned r) { return nonconst_rowref(*this, r); }
    const_rowref operator[](unsigned r) const { return const_rowref(*this, r); }

    bool operator==(const LTBitMatrix &a) const { return bitvec::operator==(a); }
    bool operator!=(const LTBitMatrix &a) const { return bitvec::operator!=(a); }
};

inline std::ostream &operator <<(std::ostream &out, const LTBitMatrix &bm) {
    for (unsigned i = 1; i < bm.size(); i++) {
        if (i > 1) out << ' ';
        for (unsigned j = 0; j < i; j++)
            out << (bm[i][j] ? '1' : '0'); }
    return out;
}

#endif /* P4C_LIB_LTBITMATRIX_H_ */
