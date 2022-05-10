
import	numpy as np
import	random as rand
import	math
import	sys

amplitude	= 0.83
offset		= 0.14
sigmax		= 2.63
sigmay		= 3.12

xlist	= np.linspace(-5.0,+5.0,50)
ylist	= np.linspace(-5.0,+5.0,50)

for j in range(len(ylist)):
	for i in range(len(xlist)):
		xval	= xlist[i]
		yval	= ylist[j]
		zval	= offset + amplitude* np.exp(-xval**2/(2.0*sigmax**2) - yval**2/(2.0*sigmay**2))
		zval	+= 0.1*(rand.random()-0.5)
		print("%.5f, %.5f, %.5f," % (xval, yval, zval))


