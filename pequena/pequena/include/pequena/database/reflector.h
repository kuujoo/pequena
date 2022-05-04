#pragma once

#define DBCOLS_BEGIN \
private: \
friend class peq::database::Reflector; \
	void peq_reflectorFromDbRow(const peq::database::SQLiteQueryResult* result, unsigned rowIndex) \
	{ \
		const auto& row = (*result)[rowIndex]; \

#define DBCOLS_SET(m) peq_reflectorSet(m, row, result->columnIndex(#m));

#define DBCOLS_END \
	} \
	template<typename T> \
	void peq_reflectorSet(T& V, const peq::database::SQLiteQueryResultRow& row, unsigned index) \
	{ \
		if(index == peq::database::SQLiteQueryResult::invalidColumn) return; \
		V = row.get<T>(index); \
	} \


#define DBCOLS1(m0) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_END \


#define DBCOLS2(m0, m1) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_END \

#define DBCOLS3(m0, m1, m2) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_END \

#define DBCOLS4(m0, m1, m2, m3) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_END \

#define DBCOLS5(m0, m1, m2, m3, m4) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_END \

#define DBCOLS6(m0, m1, m2, m3, m4, m5) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_END \


#define DBCOLS7(m0, m1, m2, m3, m4, m5, m6) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_END \

#define DBCOLS8(m0, m1, m2, m3, m4, m5, m6, m7) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_END \

#define DBCOLS9(m0, m1, m2, m3, m4, m5, m6, m7, m8) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_SET(m8) \
	DBCOLS_END \

#define DBCOLS10(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_SET(m8) \
	DBCOLS_SET(mm9) \
	DBCOLS_END \

#define DBCOLS11(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_SET(m8) \
	DBCOLS_SET(m9) \
	DBCOLS_SET(m10) \
	DBCOLS_END \

#define DBCOLS12(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_SET(m8) \
	DBCOLS_SET(m9) \
	DBCOLS_SET(m10) \
	DBCOLS_SET(m11) \
	DBCOLS_END \

#define DBCOLS13(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_SET(m8) \
	DBCOLS_SET(m9) \
	DBCOLS_SET(m10) \
	DBCOLS_SET(m11) \
	DBCOLS_SET(m12) \
	DBCOLS_END \

#define DBCOLS14(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_SET(m8) \
	DBCOLS_SET(m9) \
	DBCOLS_SET(m10) \
	DBCOLS_SET(m11) \
	DBCOLS_SET(m12) \
	DBCOLS_SET(m13) \
	DBCOLS_END \


#define DBCOLS15(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14) \
	DBCOLS_BEGIN \
	DBCOLS_SET(m0) \
	DBCOLS_SET(m1) \
	DBCOLS_SET(m2) \
	DBCOLS_SET(m3) \
	DBCOLS_SET(m4) \
	DBCOLS_SET(m5) \
	DBCOLS_SET(m6) \
	DBCOLS_SET(m7) \
	DBCOLS_SET(m8) \
	DBCOLS_SET(m9) \
	DBCOLS_SET(m10) \
	DBCOLS_SET(m11) \
	DBCOLS_SET(m12) \
	DBCOLS_SET(m13) \
	DBCOLS_SET(m14) \
	DBCOLS_END \

#define EXPAND(x) x
#define GET_DBREFLECTORMACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9,_10,_11,_12,_13,_14,_15, NAME, ...) NAME
#define DATABASE_COLUMNS(...) EXPAND(GET_DBREFLECTORMACRO(__VA_ARGS__, DBCOLS15, DBCOLS14, DBCOLS13, DBCOLS12, DBCOLS11, DBCOLS10, DBCOLS9, DBCOLS8, DBCOLS7, DBCOLS6, DBCOLS5, DBCOLS4, DBCOLS3, DBCOLS2, DBCOLS1)(__VA_ARGS__))
