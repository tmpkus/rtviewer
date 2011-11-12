/*
 * usm.h
 *
 *  Created on: Jul 7, 2011
 *      Author: janrinze
 */

#ifndef USM_H_
#define USM_H_

void sharpen (LBrBbImage & dst,float radius, float amount,float thresh);
void sharpen (LabImage & dst,float radius, float amount,float thresh);
void sharpen (HDRImage & dst,float radius, float amount,float thresh);

#endif /* USM_H_ */
