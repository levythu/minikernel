/*
 * page.h
 * Created for use in 15-410 at CMU
 * Joey Echeverria (jge)
 */

#ifndef _PAGE_H_
#define _PAGE_H_

/* Pages are 4K */

#ifndef PAGE_SHIFT
#define PAGE_SHIFT  12
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE   (1 << PAGE_SHIFT)
#endif

#endif /* _PAGE_H_ */
