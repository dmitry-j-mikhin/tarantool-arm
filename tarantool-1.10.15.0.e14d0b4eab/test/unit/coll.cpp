#include <iostream>
#include <vector>
#include <algorithm>
#include <string.h>
#include <assert.h>
#include <msgpuck.h>
#include <diag.h>
#include <fiber.h>
#include <memory.h>
#include "coll_def.h"
#include "coll.h"
#include "unit.h"
#include <PMurHash.h>

using namespace std;

enum { HASH_SEED = 13 };

struct comp {
	struct coll *coll;
	comp(struct coll *coll_) : coll(coll_) {}
	bool operator()(const char *a, const char *b) const
	{
		int cmp = coll->cmp(a, strlen(a), b, strlen(b), coll);
		return cmp < 0;
	}
};

void
test_sort_strings(vector<const char *> &strings, struct coll *coll)
{
	sort(strings.begin(), strings.end(), comp(coll));
	cout << strings[0] << endl;
	for (size_t i = 1; i < strings.size(); i++) {
		int cmp = coll->cmp(strings[i], strlen(strings[i]),
				    strings[i - 1], strlen(strings[i - 1]),
				    coll);
		cout << strings[i]
		     << (cmp < 0 ? " LESS" : cmp > 0 ? " GREATER " : " EQUAL")
		     << endl;
	}
};

void
manual_test()
{
	header();

	vector<const char *> strings;
	struct coll_def def;
	memset(&def, 0, sizeof(def));
	snprintf(def.locale, sizeof(def.locale), "%s", "ru_RU");
	def.type = COLL_TYPE_ICU;
	struct coll *coll;

	cout << " -- default ru_RU -- " << endl;
	coll = coll_new(&def);
	assert(coll != NULL);
	strings = {"Б", "бб", "е", "ЕЕЕЕ", "ё", "Ё", "и", "И", "123", "45" };
	test_sort_strings(strings, coll);
	coll_unref(coll);

	cout << " -- --||-- + upper first -- " << endl;
	def.icu.case_first = COLL_ICU_CF_UPPER_FIRST;
	coll = coll_new(&def);
	assert(coll != NULL);
	strings = {"Б", "бб", "е", "ЕЕЕЕ", "ё", "Ё", "и", "И", "123", "45" };
	test_sort_strings(strings, coll);
	coll_unref(coll);

	cout << " -- --||-- + lower first -- " << endl;
	def.icu.case_first = COLL_ICU_CF_LOWER_FIRST;
	coll = coll_new(&def);
	assert(coll != NULL);
	strings = {"Б", "бб", "е", "ЕЕЕЕ", "ё", "Ё", "и", "И", "123", "45" };
	test_sort_strings(strings, coll);
	coll_unref(coll);

	cout << " -- --||-- + secondary strength + numeric -- " << endl;
	def.icu.strength = COLL_ICU_STRENGTH_SECONDARY;
	def.icu.numeric_collation = COLL_ICU_ON;
	coll = coll_new(&def);
	assert(coll != NULL);
	strings = {"Б", "бб", "е", "ЕЕЕЕ", "ё", "Ё", "и", "И", "123", "45" };
	test_sort_strings(strings, coll);
	coll_unref(coll);

	cout << " -- --||-- + case level -- " << endl;
	def.icu.case_level = COLL_ICU_ON;
	coll = coll_new(&def);
	assert(coll != NULL);
	strings = {"Б", "бб", "е", "ЕЕЕЕ", "ё", "Ё", "и", "И", "123", "45" };
	test_sort_strings(strings, coll);
	coll_unref(coll);

	cout << " -- en_EN -- " << endl;
	snprintf(def.locale, sizeof(def.locale), "%s", "en_EN-EN");
	coll = coll_new(&def);
	assert(coll != NULL);
	strings = {"aa", "bb", "cc", "ch", "dd", "gg", "hh", "ii" };
	test_sort_strings(strings, coll);
	coll_unref(coll);

	cout << " -- cs_CZ -- " << endl;
	snprintf(def.locale, sizeof(def.locale), "%s", "cs_CZ");
	coll = coll_new(&def);
	assert(coll != NULL);
	strings = {"aa", "bb", "cc", "ch", "dd", "gg", "hh", "ii" };
	test_sort_strings(strings, coll);
	coll_unref(coll);

	footer();
}

unsigned calc_hash(const char *str, struct coll *coll)
{
	size_t str_len = strlen(str);
	uint32_t h = HASH_SEED;
	uint32_t carry = 0;
	uint32_t actual_len = coll->hash(str, str_len, &h, &carry, coll);
	return PMurHash32_Result(h, carry, actual_len);

}

void
hash_test()
{
	header();

	struct coll_def def;
	memset(&def, 0, sizeof(def));
	snprintf(def.locale, sizeof(def.locale), "%s", "ru_RU");
	def.type = COLL_TYPE_ICU;
	struct coll *coll;

	/* Case sensitive */
	coll = coll_new(&def);
	assert(coll != NULL);
	cout << "Case sensitive" << endl;
	cout << (calc_hash("ае", coll) != calc_hash("аё", coll) ? "OK" : "Fail") << endl;
	cout << (calc_hash("ае", coll) != calc_hash("аЕ", coll) ? "OK" : "Fail") << endl;
	cout << (calc_hash("аЕ", coll) != calc_hash("аё", coll) ? "OK" : "Fail") << endl;
	coll_unref(coll);

	/* Case insensitive */
	def.icu.strength = COLL_ICU_STRENGTH_SECONDARY;
	coll = coll_new(&def);
	assert(coll != NULL);
	cout << "Case insensitive" << endl;
	cout << (calc_hash("ае", coll) != calc_hash("аё", coll) ? "OK" : "Fail") << endl;
	cout << (calc_hash("ае", coll) == calc_hash("аЕ", coll) ? "OK" : "Fail") << endl;
	cout << (calc_hash("аЕ", coll) != calc_hash("аё", coll) ? "OK" : "Fail") << endl;
	coll_unref(coll);

	footer();
}

void
cache_test()
{
	header();
	plan(2);

	struct coll_def def;
	memset(&def, 0, sizeof(def));
	snprintf(def.locale, sizeof(def.locale), "%s", "ru_RU");
	def.type = COLL_TYPE_ICU;

	struct coll *coll1 = coll_new(&def);
	struct coll *coll2 = coll_new(&def);
	is(coll1, coll2,
	   "collations with the same definition are not duplicated");
	coll_unref(coll2);
	snprintf(def.locale, sizeof(def.locale), "%s", "en_EN");
	coll2 = coll_new(&def);
	isnt(coll1, coll2,
	     "collations with different definitions are different objects");
	coll_unref(coll2);
	coll_unref(coll1);

	check_plan();
	footer();
}

int
main(int, const char**)
{
	coll_init();
	memory_init();
	fiber_init(fiber_c_invoke);
	manual_test();
	hash_test();
	cache_test();
	fiber_free();
	memory_free();
	coll_free();
}
