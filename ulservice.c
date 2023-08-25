#include "ulservice.h"












char *ul_strstr(const char *s1, const char *s2)
{
	int l1, l2;
	l2 = ul_strlen(s2);
	if(!l2)
		return (char *)s1;
	l1 = ul_strlen(s1);
	while(l1 >= l2)
	{
		l1 --;
		if(!ul_memcmp(s1, s2, l2))
			return (char *)s1;
		s1 ++;
	}
	return UL_NULL;
}



/* ul_strlen
 * 返回字符串长度,通过找'\0'符
 * return：	字符串长度
 */
ul_size_t ul_strlen(const char *s)
{
	const char *sc;	/*指针里面内容不可变,指针可以变*/
	for(sc = s; *sc != '\0'; ++sc)
		;
	return sc - s;
}




/* ul_strncmp
 * 比较两个字符串是否相同
 * cs:		字符串1
 * ct：		字符串2
 * count：	要比较的个数
 * return：相同返回0,否则返回其他
 */
ul_int32_t ul_strncmp(const char *cs, const char *ct, ul_ubase_t count)
{
	register signed char __res = 0;
	while(count)
	{
		if((__res = *cs - *ct++) != 0 || !*cs++)
			break;
		count --;
	}
	return __res;
}


