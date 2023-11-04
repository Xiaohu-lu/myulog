#include "ulservice.h"
#include <stdarg.h>



/* ul_memcmp
 * �Ƚ������ַ����Ƿ���ͬ
 * cs:		�ַ���1
 * ct��		�ַ���2
 * count��	Ҫ�Ƚϵĸ���
 * return����ͬ����0,���򷵻�����
 */
ul_int32_t ul_memcmp(const void *cs, const void *ct, ul_ubase_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;
	for(su1 = (const unsigned char *)cs, su2 = (const unsigned char *)ct; 0 < count; ++su1, ++su2, count--)
		if((res = *su1 - *su2) != 0)
			break;
	return res;
}






/* ul_strstr
 * s1:		Դ�ַ���
 * s2��		Ҫ�ҵ��ַ���
 * return:	����s2��һ����s1�г��ֵ�λ��
 */
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



/* ul_strncpy
 * ��src����n�ֽڵ�dst
 * dst��		Ŀ�ĵ�ַ
 * src��		Դ���ݵ�ַ
 * n��		Ҫ�������ֽ���
 */
char *ul_strncpy(char *dst, const char *src, ul_ubase_t n)
{
	if(n != 0)
	{
		char *d = dst;
		const char *s = src;
		do
		{
			if((*d++ = *s++) == 0)
			{
				while(--n != 0)
					*d++ = 0;
				break;
			}
		
		}
		while (--n != 0);
	}
	return (dst);
}





/* ul_strlen
 * �����ַ�������,ͨ����'\0'��
 * return��	�ַ�������
 */
ul_size_t ul_strlen(const char *s)
{
	const char *sc;	/*ָ���������ݲ��ɱ�,ָ����Ա�*/
	for(sc = s; *sc != '\0'; ++sc)
		;
	return sc - s;
}




/* ul_strncmp
 * �Ƚ������ַ����Ƿ���ͬ
 * cs:		�ַ���1
 * ct��		�ַ���2
 * count��	Ҫ�Ƚϵĸ���
 * return����ͬ����0,���򷵻�����
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



/* private function */
/* �ж��ַ��ǲ�������'0123456789'
 */
#define _ISDIGIT(c)		((unsigned)((c) - '0') < 10)	




#define ZEROPAD			(1 << 0)		/* pad with zero */
#define SIGN			(1 << 1)		/* unsigned/signed long */
#define PLUS			(1 << 2)		/* show plus */
#define SPACE			(1 << 3)		/* space if plus */
#define LEFT			(1 << 4)		/* left justified */
#define SPECIAL			(1 << 5)		/* 0x */
#define LARGE			(1 << 6)		/*use 'ABCDEF' instead of 'abcdef' */


/* skip_atoi
 * �ַ���ת��Ϊint����
 * s��	Ҫת�����ַ�����ַ
 * return��	ת���������
 */
ul_inline int skip_atoi(const char **s)
{
	register int i = 0;
	while(_ISDIGIT(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

#ifdef RT_PRINTF_LONGLONG
ul_inline int divide(long long *n, int base)
{
    int res;

    /* optimized for processor which does not support divide instructions. */
    if (base == 10)
    {
        res = (int)(((unsigned long long)*n) % 10U);
        *n = (long long)(((unsigned long long)*n) / 10U);
    }
    else
    {
        res = (int)(((unsigned long long)*n) % 16U);
        *n = (long long)(((unsigned long long)*n) / 16U);
    }

    return res;
}
#else
ul_inline int divide(long *n, int base)
{
    int res;

    /* optimized for processor which does not support divide instructions. */
    if (base == 10)
    {
        res = (int)(((unsigned long)*n) % 10U);
        *n = (long)(((unsigned long)*n) / 10U);
    }
    else
    {
        res = (int)(((unsigned long)*n) % 16U);
        *n = (long)(((unsigned long)*n) / 16U);
    }

    return res;
}
#endif


#ifdef UL_PRINTF_PRECISION
static char *print_number(char *buf, 
							   char *end,
#ifdef UL_PRINTF_LONGLONG
							   long long num,
#else
							   long num,
#endif	/*UL_PRINTF_LONGLONG*/
							   int base,
							   int s,
							   int precision,
							   int type)
#else
static char *print_number(char *buf,
						  char *end,
#ifdef UL_PRINTF_LONGLONG
						  long long num,
#else
						  long num,
#endif	/*UL_PRINTF_LONGLONG*/
						  int base,
						  int s,
						  int type)
#endif /*UL_PRINTF_PRECISION*/
{
	char c, sign;
#ifdef UL_PRINTF_LONGLONG
	char tmp[32];
#else
	char tmp[16];
#endif
#ifdef UL_PRINTF_PRECISION
	int precision_bak = precision;
#endif /*UL_PRINTF_PRECISION*/
	const char *digits;
	static const char small_digits[] = "0123456789abcdef";
	static const char large_digits[] = "0123456789ABCDEF";
	register int i;
	register int size;

	size = s;
	digits = (type & LARGE) ? large_digits : small_digits;

	if(type & LEFT)
		type &= ~ZEROPAD;

	c = (type & ZEROPAD) ? '0' : ' ';

	/*get sign*/
	sign = 0;
	if(type & SIGN)
	{
		if(num < 0)
		{
			sign = '-';
			num = -num;
		}
		else if(type & PLUS)
			sign = '+';
		else if(type & SPACE)
			sign = ' ';
	}

#ifdef UL_PRINTF_SPECIAL
	if(type & SPECIAL)
	{
		if(base == 16)
			size -= 2;
		else if(base == 8)
			size--;
	}
#endif	/*UL_PRINTF_SPECIAL*/
	
	i = 0;
	if(num == 0)
		tmp[i++] = '0';
	else 
	{
		while(num != 0)
			tmp[i++] = digits[divide(&num, base)];
	}
#ifdef UL_PRINTF_PRECISION
	if(i > precision)
		precision = i;
	size -= precision;
#else
 	size -= i;
#endif /*UL_PRINTF_PRECISION*/

	if(!(type & (ZEROPAD | LEFT)))
	{
		if((sign) && (size > 0))
			size-- ;
		while(size-- > 0
)
		{
			if(buf < end)
				*buf = ' ';
			++ buf;
		}
	}

	if(sign)
	{
		if(buf < end)
		{
			*buf = sign;
		}
		-- size;
		++ buf;
	}

#ifdef UL_PRINTF_SPECIAL
	if(type & SPECIAL)
	{
		if(base == 8)
		{
			if(buf < end)
				*buf = '0';
			++ buf;
		}
		else if(base == 16)
		{
			if(buf < end)
				*buf = '0';
			++ buf;
			if(buf < end)
			{
				*buf = type & LARGE ? 'X' : 'x';
			}
			++ buf;
		}
	}
#endif /*UL_PRINTF_SPECIAL*/

	if(!(type & LEFT))
	{
		while(size-- > 0)
		{
			if(buf < end)
				*buf = c;
			++ buf;
		}
	}

#ifdef UL_PRINTF_PRECISION
	while(i < precision--)
	{
		if(buf < end)
			*buf = '0';
		++ buf;
	}


	while(i-- > 0 && (precision_bak != 0))
	{
		if(buf < end)
			*buf = tmp[i];
		++ buf;
	}
#endif /*UL_PRINTF_PRECISION*/

	while(size-- > 0)
	{
	 	if(buf < end)
			*buf = ' ';
		++ buf;
	}

	return buf;

}



/* ul_vsnprintf
 * ��ʽ�������buf������,%[flags][width][.precision][length]soecifier
 * buf:		��ʽ��Ļ�����
 * size��	buf������ɵ��ַ�������
 * fmt��		��ʽ
 * args��	�ɱ��β��б�
 * return��	��ʽ��������ַ�����
 */
ul_int32_t ul_vsnprintf(char *buf, ul_size_t size, const char *fmt, va_list args)
{
#ifdef UL_PRINTF_LONGLONG
	unsigned long long num;
#else
	ul_uint32_t num;
#endif	/*UL_PRINTF_LONGLONG*/
	int i, len;
	char *str, *end, c;
	const char *s;

	ul_uint8_t base;
	ul_uint8_t flags;
	ul_uint8_t qualifier;
	ul_int32_t field_width;

#ifdef UL_PRINTF_PRECISION
	int precision;
#endif	/*UL_PRINTF_PRECISION*/

	str = buf;
	end = buf + size;

	/* ȷ��end���Ǵ���buf
	 */
	if(end < buf)
	{
		end = ((char*) -1);
		size = end - buf;
	}

	for(; *fmt; ++fmt)
	{
		if(*fmt != '%')
		{
			if(str < end)
				*str = *fmt;
			++ str;
			continue;
		}

		/* flags ��
		 * a,A����16������ʽ���������
		 * d��	��10�����������������(�������������)
		 * o:	��8��������޷�������
		 * x,X:	��10��������޷�������
		 * u:	��10��������޷�������
		 * f��	��С����ʽ�������˫����ʵ��
		 * ...
 		 */
		flags = 0;
		while(1)
		{
			/* ����%����
 		     */
 		    ++ fmt;
			if(*fmt == '-') flags |= LEFT;
			else if(*fmt == '+') flags |= PLUS;
			else if(*fmt == ' ') flags |= SPACE;
			else if(*fmt == '#') flags |= SPECIAL;
			else if(*fmt == '0') flags |= ZEROPAD;
			else break;
		}

		/* width:
		 * ����ַ�������ǰ������ո�
		 */
		/* get field width */
		field_width = -1;
		if(_ISDIGIT(*fmt)) field_width = skip_atoi(&fmt);
		else if(*fmt == '*')
		{
			++ fmt;
			field_width = va_arg(args, int);
			if(field_width < 0)
			{
				field_width = -field_width;
				flags |= LEFT;
			}
		}
#ifdef UL_PRINTF_PRECISION
		/* get the precision */
		precision = -1;
		if(*fmt == '.')
		{
			++fmt;
			if(_ISDIGIT(*fmt)) precision = skip_atoi(&fmt);
			else if(*fmt == '*')
			{
				++ fmt;
				precision = va_arg(args, int);
			}
			if(precision < 0) precision = 0;
		}
#endif

		/* get the conversion qualifier */
		qualifier = 0;
#ifdef UL_PRINTF_LONGLONG
		if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
#else
		if(*fmt == 'h' || *fmt == 'l')
#endif	/*UL_PRINTF_LONGLONG*/
		{
			qualifier = *fmt;
			++ fmt;
#ifdef UL_PRINTF_LONGLONG
			if(qualifier == 'l' && *fmt == 'l')
			{
				qualifier = 'L';
				++ fmt;
			}
#endif	/*UL_PRINTF_LONGLONG*/
		}

		/* the default base */
		base = 10;
		switch(*fmt)
		{
			case 'c':
				if(!(flags & LEFT))
				{
					while(--field_width > 0)
					{
						if(str < end) *str = ' ';
						++ str;
					}
				}
				/* get character */
				c = (ul_uint8_t)va_arg(args, int);
				if(str < end) *str = c;
				++ str;
				/* put width */
				while(--field_width > 0)
				{
					if(str < end) *str = ' ';
					++ str;
				}
				continue;

			case 's':
				s = va_arg(args, char *);
				if(!s) s = "(NULL)";
				for(len = 0; (len != field_width) && (s[len] != '\0'); len++);
#ifdef UL_PRINTF_PRECISION
				if(precision > 0 && len > precision) len = precision;
#endif /* UL_PRINTF_PRECISION */
				if(!(flags & LEFT))
				{
					while(len < field_width--)
					{
						if(str < end) *str = ' ';
						++ str;
					}
				}

				for(i = 0; i < len; ++i)
				{
					if(str < end) *str = *s;
					++ str;
					++ s;
				}
				while(len < field_width--)
				{
					if(str < end) *str = ' ';
					++ str;
				}
				continue;
			case 'p':
				if(field_width == -1)
				{
					field_width = sizeof(void *) << 1;
					flags |= ZEROPAD;
				}
#ifdef UL_PRINTF_PRECISION
				str = print_number(str, end, (long)va_arg(args, void *), 16, field_width, precision, flags);
#else 
				str = print_number(str, end, (long)va_arg(args, void *), 16, field_width, flags);
#endif	/* UL_PRINTF_PRECISION */
				continue;
			case '%':
				if(str < end) *str = '%';
				++ str;
				continue;

			case 'o':
				base = 8;
				break;
			case 'X':
				flags |= LARGE;
			case 'x':
				base = 16;
				break;
			case 'd':
			case 'i':
				flags |= SIGN;
			case 'u':
				break;
			default:
				if(str < end) *str = '%';
				++ str;
				if(*fmt)
				{
					if(str < end) *str = *fmt;
					++ str;
				}
				else
				{
					-- fmt;
				}
				continue;
				
		}
#ifdef UL_PRINTF_LONGLONG
		if(qualifier == 'L') num = va_arg(args, long long);
		else if(qualifier == 'l')
#else 
		if(qualifier == 'l')
#endif /* UL_PRINTF_LONGLONG */
		{
			num = va_arg(args, ul_uint32_t);
			if(flags & SIGN) num = (ul_int32_t)num;
		}
		else if(qualifier == 'h')
		{
			num = (ul_uint16_t)va_arg(args, ul_int32_t);
			if(flags & SIGN) num = (ul_int16_t)num;
		}
		else
		{
			num = va_arg(args, ul_uint32_t);
			if(flags & SIGN) num = (ul_int32_t)num;
		}
#ifdef UL_PRINTF_PRECISION
		str = print_number(str, end, num, base, field_width, precision, flags);
#else 
		str = print_number(str, end, num, base, field_width, flags);
#endif	/*UL_PRINTF_RECISION*/
	
	}

	if(size > 0)
	{
		if(str < end) *str = '\0';
		else
		{
			end[-1] = '\0';
		}
	}
	
	return str - buf;
}

/* ul_snprintf
 * ��һ����ʽ�����ַ�����䵽buffer��
 * buf��	������
 * size����������С
 * fmt����ʽ
 * ...���β�
 */
ul_int32_t ul_snprintf(char *buf, ul_size_t size, const char *fmt, ...)
{
	ul_int32_t n;
	va_list args;

	va_start(args, fmt);
	n = ul_vsnprintf(buf, size, fmt, args);
	va_end(args);

	return n;
}






