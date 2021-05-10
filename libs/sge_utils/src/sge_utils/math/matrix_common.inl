/////////////////////////////////////////////////////////////////
//[TODO][NOTE] Column those operations are
// not ready for column major operation yet!!!
// Probably a local IS_COLUMN_MAJOR-ish macro will take place
// since there are few differences.
//
// This *.inl contains common matrix operation.
// The file is shared between several matrix classes.
// Each class that uses that file should have defined as members:
//
// SELF_TYPE - typedev <CLASS_NAME> SELF_TYPE;
// DATA_TYPE - the data storage type
// NUM_ROW and NUM_COL - matrix size in mathematical meaning
// VEC_TYPE - storage vector type for (row/column)
// data[] - an array of type VEC_TYPE with size NUM_VECS
// NUM_VECS - the number of VEC_TYPEs needed to represent the matrix
// VEC_SIZE - the number of data elements in VEC_TYPE
/////////////////////////////////////////////////////////////////

//---------------------------------------------------
// Member access methods
//[NOTE]operator[] isn't overloaded because of
// the mixed nature of the matrix storage layout.
//---------------------------------------------------

bool operator==(const SELF_TYPE& other) const {
	bool v = true;
	for (int t = 0; t < NUM_VECS; ++t) {
		v &= data[t] == other.data[t];
	}

	return v;
}

bool operator!=(const SELF_TYPE& other) const {
	bool v = true;
	for (int t = 0; t < NUM_VECS; ++t) {
		v &= data[t] != other.data[t];
	}

	return v;
}

//---------------------------------------------------
//
//---------------------------------------------------
static SELF_TYPE getDiagonal(const DATA_TYPE& s) {
	SELF_TYPE result;
	for (int t = 0; t < NUM_VECS; ++t) {
		result.data[t] = VEC_TYPE::getAxis(t, s);
	}

	return result;
}

//---------------------------------------------------
//
//---------------------------------------------------
static SELF_TYPE getIdentity() {
	return getDiagonal((DATA_TYPE)1.0);
}

//---------------------------------------------------
//
//---------------------------------------------------
static SELF_TYPE getZero() {
	return getDiagonal((DATA_TYPE)0.0);
}

//---------------------------------------------------
//
//---------------------------------------------------
VEC_TYPE getRow(int iRow) const {
	sgeAssert(iRow >= 0 && iRow < NUM_VECS);
	VEC_TYPE result;

	for (int t = 0; t < NUM_VECS; ++t) {
		result.data[t] = data[t][iRow];
	}

	return result;
}

DATA_TYPE& at(unsigned const int r, const unsigned int c) {
	return data[c][r];
}

const DATA_TYPE& at(unsigned const int r, const unsigned int c) const {
	return data[c][r];
}

void identifyAxis(const unsigned int i) {
	data[i] = VEC_TYPE::get_axis(i);
}

//---------------------------------------------------
// Scalars operator* and *=
//---------------------------------------------------
SELF_TYPE& operator*=(const DATA_TYPE& s) {
	for (unsigned int t = 0; t < NUM_VECS; ++t) {
		data[t] *= s;
	}
	return *this;
}

SELF_TYPE operator*(const DATA_TYPE& s) const {
	SELF_TYPE result(*this);
	result *= s;
	return result;
}

friend SELF_TYPE operator*(const DATA_TYPE& s, const SELF_TYPE& m) {
	return m * s;
}

//---------------------------------------------------
// operator+ and +=
//---------------------------------------------------
SELF_TYPE& operator+=(const SELF_TYPE& m) {
	for (unsigned int t = 0; t < NUM_VECS; ++t) {
		data[t] += m.data[t];
	}
	return *this;
}

SELF_TYPE operator+(const SELF_TYPE& m) const {
	SELF_TYPE result(*this);
	result += m;
	return result;
}

//---------------------------------------------------
// operator- and -=
//---------------------------------------------------
SELF_TYPE& operator-=(const SELF_TYPE& m) {
	SELF_TYPE result;
	for (unsigned int t = 0; t < NUM_VECS; ++t) {
		data[t] -= m.data[t];
	}
	return *this;
}

SELF_TYPE operator-(const SELF_TYPE& m) const {
	SELF_TYPE result(*this);
	result -= m;
	return result;
}

//---------------------------------------------------
// Vector * Matrix  -> (1, NUM_COL) (a ROW vector)
//---------------------------------------------------
friend VEC_TYPE operator*(const VEC_TYPE& v, const SELF_TYPE& m) {
	VEC_TYPE result;
	for (int t = 0; t < NUM_VECS; ++t) {
		result[t] = dot(v, m.data[t]);
	}
	return result;
}

//---------------------------------------------------
// Matrix * Vector -> (NUM_ROW, 1) (a COLUMN vector)
//---------------------------------------------------
friend VEC_TYPE operator*(const SELF_TYPE& m, const VEC_TYPE& v) {
	VEC_TYPE result = VEC_TYPE::getZero();

	for (int t = 0; t < NUM_VECS; ++t) {
		result += m.data[t] * v[t];
	}

	return result;
}

//---------------------------------------------------
// Matrix * Matrix mathematical multiplication
//---------------------------------------------------
friend SELF_TYPE operator*(const SELF_TYPE& a, const SELF_TYPE& b) {
	SELF_TYPE r;
	for (unsigned int t = 0; t < NUM_VECS; ++t) {
		r.data[t] = a * b.data[t];
	}

	return r;
}

//---------------------------------------------------
// Matrix * Matrix componentwise multiplication
//---------------------------------------------------
friend SELF_TYPE cmul(const SELF_TYPE& a, const SELF_TYPE& b) {
	SELF_TYPE r;
	for (unsigned int t = 0; t < NUM_VECS; ++t) {
		r.data[t] = cmul(a.data[t], b.data[t]);
	}
	return r;
}

//---------------------------------------------------
// transpose
//---------------------------------------------------
SELF_TYPE transposed() const {
	SELF_TYPE result;

	for (int t = 0; t < NUM_ROW; ++t)
		for (int s = 0; s < NUM_COL; ++s) {
			result.at(t, s) = at(s, t);
		}

	return result;
}

friend SELF_TYPE transposed(const SELF_TYPE& m) {
	return m.transposed();
}
