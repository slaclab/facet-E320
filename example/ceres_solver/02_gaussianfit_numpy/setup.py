#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <https://www.gnu.org/licenses/>.
# 
# Copyright Sebastian Meuren, 2022
# 

from distutils.core import setup, Extension
import numpy as np 


module = Extension('twodimgaussianfit',
	sources = ['np_interface.cc'],
	libraries=['glog', 'lapack'],
	include_dirs=[np.get_include(),np.get_include()+'/numpy',"/usr/local/include/eigen3/"],
	extra_objects=['/usr/local/lib/libceres.a'])

setup(name = '2D Gaussian Fit',
		version			= '0.01',
		description		= 'Performs a 2D Gaussian fit',
		author			= 'Sebastian Meuren',
		author_email	= 'smeuren@stanford.edu',
		ext_modules		= [module])

