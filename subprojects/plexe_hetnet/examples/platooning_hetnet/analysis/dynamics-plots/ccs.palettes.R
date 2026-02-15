# Original fireprint palette is Copyright 2011 Matthias Geissbuehler - matthias.geissbuehler@a3.epfl.ch
#
# Palette generator for R graphs
# Copyright (C) 2013 Michele Segata, CCS Labs <segata@ccs-labs.org> 
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

ratio.to.n <- function(ratio) {
	return (ratio * 255 + 1)
}
n.to.ratio <- function(n) {
	return ((n - 1) / 255)
}

# function for getting CCS fireprint color palette, which is good for color printing,
# B/W printing, and colorblind safe (mostly... see [1]).
# the min and max parameters can be used to set which color in the original
# palette the returned palette should start from or end to. the entire palette
# indeed goes from black to white and might not be suitable for printing curves.
# for example, by setting n=10, min=0, and max=1, first color will be black
# and last will be white (or similar)
# by setting n=10, min=0, and max=.5, first color will be black and last will
# be magenta/violet, making it suitable for plotting curves.
# obviously, by changing min and max values, the total number of available
# colors will also change. setting n=256, min=0, and max=.5 will raise an
# error
#
# param: n number of colors to be put into the returned palette. The palette
# supports up to 256 colors (default 256)
# param: min minimum color of the full palette to be used (default 0)
# param: max maximum color of the full palette to be used (default 1)
#
# [1] Fireprint palette: http://www.mathworks.com/matlabcentral/fileexchange/31761-colormap-fire-optimized-for-print
get.ccs.fireprint.palette <- function(n=256, min=0, max=1) {
	#compute the number of available colors
	n.color <- floor(ratio.to.n(max) - ratio.to.n(min) + 1)
	#make some safety checks
	if ((max < min) || (min < 0) || (max > 1)) {
		stop(paste('invalid min and max values specified: min =', min, 'max =', max, '\ncheck that min >= 0, max <= 1, and max >= min'))
	}
	if (n < 1 || n > n.color) {
		stop(paste('palette size must be between 1 and', n.color, ' while', n, 'has been given'))
	}
	#definition of full 256 colors palette
	color.palette <- c(
		rgb(0.00000, 0.00000, 0.00000),
		rgb(0.00000, 0.00340, 0.00589),
		rgb(0.00000, 0.00679, 0.01178),
		rgb(0.00000, 0.01019, 0.01766),
		rgb(0.00000, 0.01358, 0.02355),
		rgb(0.00000, 0.01698, 0.02944),
		rgb(0.00000, 0.02038, 0.03533),
		rgb(0.00000, 0.02377, 0.04122),
		rgb(0.00000, 0.02717, 0.04710),
		rgb(0.00000, 0.03057, 0.05299),
		rgb(0.00000, 0.03396, 0.05888),
		rgb(0.00000, 0.03736, 0.06477),
		rgb(0.00000, 0.04075, 0.07066),
		rgb(0.00000, 0.04415, 0.07654),
		rgb(0.00000, 0.04755, 0.08243),
		rgb(0.00000, 0.05094, 0.08832),
		rgb(0.00000, 0.05434, 0.09421),
		rgb(0.00000, 0.05774, 0.10010),
		rgb(0.00000, 0.06113, 0.10598),
		rgb(0.00000, 0.06453, 0.11187),
		rgb(0.00000, 0.06792, 0.11776),
		rgb(0.00000, 0.07132, 0.12365),
		rgb(0.00000, 0.07472, 0.12953),
		rgb(0.00000, 0.07811, 0.13542),
		rgb(0.00000, 0.08151, 0.14131),
		rgb(0.00000, 0.08490, 0.14720),
		rgb(0.00000, 0.08830, 0.15309),
		rgb(0.00000, 0.09170, 0.15897),
		rgb(0.00000, 0.09509, 0.16486),
		rgb(0.00000, 0.09849, 0.17075),
		rgb(0.00000, 0.10189, 0.17664),
		rgb(0.00000, 0.10528, 0.18253),
		rgb(0.00000, 0.10868, 0.18841),
		rgb(0.00000, 0.11207, 0.19430),
		rgb(0.00000, 0.11547, 0.20019),
		rgb(0.00000, 0.11887, 0.20608),
		rgb(0.00000, 0.12226, 0.21197),
		rgb(0.00000, 0.12566, 0.21785),
		rgb(0.00000, 0.12906, 0.22374),
		rgb(0.00000, 0.13245, 0.22963),
		rgb(0.00000, 0.13585, 0.23552),
		rgb(0.00000, 0.13924, 0.24141),
		rgb(0.00000, 0.14264, 0.24729),
		rgb(0.00000, 0.14604, 0.25318),
		rgb(0.00000, 0.14943, 0.25907),
		rgb(0.00000, 0.15283, 0.26496),
		rgb(0.00000, 0.15622, 0.27085),
		rgb(0.00000, 0.15962, 0.27673),
		rgb(0.00000, 0.16302, 0.28262),
		rgb(0.00000, 0.16641, 0.28851),
		rgb(0.00000, 0.16981, 0.29440),
		rgb(0.00000, 0.17321, 0.30029),
		rgb(0.00000, 0.17660, 0.30617),
		rgb(0.00000, 0.18000, 0.31206),
		rgb(0.00000, 0.18339, 0.31795),
		rgb(0.00000, 0.18679, 0.32384),
		rgb(0.00000, 0.19019, 0.32973),
		rgb(0.00000, 0.19358, 0.33561),
		rgb(0.00000, 0.19698, 0.34150),
		rgb(0.00000, 0.20038, 0.34739),
		rgb(0.00000, 0.20377, 0.35328),
		rgb(0.00000, 0.20717, 0.35916),
		rgb(0.00000, 0.21056, 0.36505),
		rgb(0.00000, 0.21396, 0.37094),
		rgb(0.00115, 0.21733, 0.37684),
		rgb(0.00589, 0.22061, 0.38275),
		rgb(0.01090, 0.22385, 0.38861),
		rgb(0.01618, 0.22706, 0.39442),
		rgb(0.02172, 0.23023, 0.40018),
		rgb(0.02754, 0.23336, 0.40587),
		rgb(0.03365, 0.23644, 0.41150),
		rgb(0.04003, 0.23946, 0.41706),
		rgb(0.04671, 0.24244, 0.42253),
		rgb(0.05368, 0.24535, 0.42791),
		rgb(0.06094, 0.24819, 0.43321),
		rgb(0.06850, 0.25096, 0.43839),
		rgb(0.07636, 0.25366, 0.44347),
		rgb(0.08453, 0.25627, 0.44844),
		rgb(0.09299, 0.25880, 0.45327),
		rgb(0.10177, 0.26123, 0.45798),
		rgb(0.11085, 0.26356, 0.46254),
		rgb(0.12024, 0.26577, 0.46696),
		rgb(0.12993, 0.26787, 0.47122),
		rgb(0.13993, 0.26985, 0.47531),
		rgb(0.15024, 0.27168, 0.47923),
		rgb(0.16086, 0.27338, 0.48297),
		rgb(0.17177, 0.27491, 0.48652),
		rgb(0.18299, 0.27629, 0.48987),
		rgb(0.19451, 0.27748, 0.49302),
		rgb(0.20633, 0.27849, 0.49595),
		rgb(0.21844, 0.27930, 0.49866),
		rgb(0.23083, 0.27989, 0.50114),
		rgb(0.24352, 0.28027, 0.50338),
		rgb(0.25648, 0.28040, 0.50538),
		rgb(0.26971, 0.28028, 0.50712),
		rgb(0.28321, 0.27990, 0.50859),
		rgb(0.29697, 0.27925, 0.50979),
		rgb(0.31097, 0.27831, 0.51071),
		rgb(0.32522, 0.27708, 0.51134),
		rgb(0.33968, 0.27554, 0.51167),
		rgb(0.35436, 0.27370, 0.51170),
		rgb(0.36923, 0.27153, 0.51141),
		rgb(0.38428, 0.26904, 0.51080),
		rgb(0.39950, 0.26622, 0.50985),
		rgb(0.41485, 0.26308, 0.50858),
		rgb(0.43032, 0.25962, 0.50697),
		rgb(0.44589, 0.25583, 0.50501),
		rgb(0.46153, 0.25174, 0.50272),
		rgb(0.47723, 0.24733, 0.50008),
		rgb(0.49294, 0.24263, 0.49710),
		rgb(0.50865, 0.23765, 0.49380),
		rgb(0.52433, 0.23240, 0.49016),
		rgb(0.53995, 0.22689, 0.48621),
		rgb(0.55550, 0.22114, 0.48195),
		rgb(0.57094, 0.21517, 0.47740),
		rgb(0.58625, 0.20900, 0.47258),
		rgb(0.60142, 0.20263, 0.46749),
		rgb(0.61642, 0.19610, 0.46215),
		rgb(0.63123, 0.18940, 0.45659),
		rgb(0.64585, 0.18257, 0.45082),
		rgb(0.66026, 0.17561, 0.44485),
		rgb(0.67445, 0.16854, 0.43871),
		rgb(0.68842, 0.16137, 0.43242),
		rgb(0.70214, 0.15412, 0.42597),
		rgb(0.71563, 0.14679, 0.41941),
		rgb(0.72888, 0.13939, 0.41272),
		rgb(0.74189, 0.13193, 0.40594),
		rgb(0.75466, 0.12443, 0.39907),
		rgb(0.76517, 0.12699, 0.39293),
		rgb(0.77342, 0.13968, 0.38750),
		rgb(0.78144, 0.15239, 0.38198),
		rgb(0.78924, 0.16512, 0.37638),
		rgb(0.79680, 0.17787, 0.37070),
		rgb(0.80414, 0.19063, 0.36495),
		rgb(0.81125, 0.20340, 0.35914),
		rgb(0.81813, 0.21618, 0.35327),
		rgb(0.82480, 0.22897, 0.34736),
		rgb(0.83124, 0.24176, 0.34140),
		rgb(0.83747, 0.25456, 0.33540),
		rgb(0.84348, 0.26737, 0.32937),
		rgb(0.84928, 0.28017, 0.32330),
		rgb(0.85487, 0.29298, 0.31721),
		rgb(0.86025, 0.30579, 0.31109),
		rgb(0.86542, 0.31860, 0.30495),
		rgb(0.87039, 0.33142, 0.29878),
		rgb(0.87517, 0.34423, 0.29260),
		rgb(0.87974, 0.35704, 0.28640),
		rgb(0.88412, 0.36985, 0.28018),
		rgb(0.88832, 0.38265, 0.27394),
		rgb(0.89232, 0.39545, 0.26769),
		rgb(0.89613, 0.40825, 0.26143),
		rgb(0.89976, 0.42103, 0.25515),
		rgb(0.90322, 0.43381, 0.24886),
		rgb(0.90649, 0.44658, 0.24257),
		rgb(0.90959, 0.45933, 0.23626),
		rgb(0.91252, 0.47207, 0.22994),
		rgb(0.91528, 0.48479, 0.22361),
		rgb(0.91787, 0.49749, 0.21727),
		rgb(0.92031, 0.51017, 0.21092),
		rgb(0.92258, 0.52283, 0.20457),
		rgb(0.92470, 0.53546, 0.19821),
		rgb(0.92667, 0.54806, 0.19185),
		rgb(0.92849, 0.56062, 0.18548),
		rgb(0.93017, 0.57315, 0.17910),
		rgb(0.93170, 0.58564, 0.17272),
		rgb(0.93310, 0.59809, 0.16634),
		rgb(0.93437, 0.61050, 0.15996),
		rgb(0.93551, 0.62286, 0.15357),
		rgb(0.93652, 0.63517, 0.14719),
		rgb(0.93742, 0.64742, 0.14080),
		rgb(0.93820, 0.65962, 0.13442),
		rgb(0.93886, 0.67177, 0.12803),
		rgb(0.93943, 0.68385, 0.12165),
		rgb(0.93989, 0.69586, 0.11527),
		rgb(0.94025, 0.70781, 0.10890),
		rgb(0.94052, 0.71969, 0.10252),
		rgb(0.94070, 0.73150, 0.09616),
		rgb(0.94080, 0.74324, 0.08980),
		rgb(0.94082, 0.75490, 0.08344),
		rgb(0.94076, 0.76649, 0.07709),
		rgb(0.94063, 0.77799, 0.07075),
		rgb(0.94043, 0.78942, 0.06442),
		rgb(0.94017, 0.80077, 0.05809),
		rgb(0.93985, 0.81203, 0.05177),
		rgb(0.93948, 0.82321, 0.04546),
		rgb(0.93906, 0.83430, 0.03916),
		rgb(0.93858, 0.84531, 0.03287),
		rgb(0.93807, 0.85624, 0.02659),
		rgb(0.93751, 0.86707, 0.02032),
		rgb(0.93692, 0.87782, 0.01405),
		rgb(0.93629, 0.88849, 0.00780),
		rgb(0.93563, 0.89907, 0.00156),
		rgb(0.93976, 0.90449, 0.01159),
		rgb(0.94534, 0.90812, 0.02707),
		rgb(0.95075, 0.91167, 0.04260),
		rgb(0.95600, 0.91513, 0.05817),
		rgb(0.96108, 0.91851, 0.07378),
		rgb(0.96600, 0.92180, 0.08942),
		rgb(0.97074, 0.92501, 0.10510),
		rgb(0.97532, 0.92815, 0.12080),
		rgb(0.97973, 0.93121, 0.13654),
		rgb(0.98397, 0.93420, 0.15231),
		rgb(0.98804, 0.93712, 0.16811),
		rgb(0.99194, 0.93996, 0.18393),
		rgb(0.99568, 0.94274, 0.19978),
		rgb(0.99924, 0.94546, 0.21564),
		rgb(0.99386, 0.94751, 0.26947),
		rgb(0.99613, 0.95066, 0.28468),
		rgb(0.99829, 0.95372, 0.29991),
		rgb(0.99189, 0.95483, 0.34860),
		rgb(0.99323, 0.95798, 0.36317),
		rgb(0.99450, 0.96103, 0.37775),
		rgb(0.99568, 0.96396, 0.39235),
		rgb(0.99679, 0.96680, 0.40696),
		rgb(0.99783, 0.96952, 0.42159),
		rgb(0.99879, 0.97215, 0.43622),
		rgb(0.99968, 0.97467, 0.45085),
		rgb(0.99394, 0.97493, 0.49129),
		rgb(0.99445, 0.97729, 0.50522),
		rgb(0.99494, 0.97955, 0.51915),
		rgb(0.99539, 0.98170, 0.53308),
		rgb(0.99581, 0.98375, 0.54700),
		rgb(0.99620, 0.98569, 0.56091),
		rgb(0.99656, 0.98753, 0.57481),
		rgb(0.99689, 0.98926, 0.58869),
		rgb(0.99720, 0.99089, 0.60256),
		rgb(0.99748, 0.99242, 0.61641),
		rgb(0.99774, 0.99385, 0.63023),
		rgb(0.99797, 0.99518, 0.64404),
		rgb(0.99819, 0.99641, 0.65781),
		rgb(0.99838, 0.99755, 0.67156),
		rgb(0.99856, 0.99859, 0.68528),
		rgb(0.99460, 0.99674, 0.71292),
		rgb(0.99520, 0.99709, 0.72598),
		rgb(0.99575, 0.99741, 0.73899),
		rgb(0.99626, 0.99771, 0.75197),
		rgb(0.99351, 0.99598, 0.77481),
		rgb(0.99407, 0.99632, 0.78715),
		rgb(0.99460, 0.99663, 0.79945),
		rgb(0.99509, 0.99693, 0.81170),
		rgb(0.99556, 0.99721, 0.82390),
		rgb(0.99600, 0.99747, 0.83605),
		rgb(0.99641, 0.99772, 0.84815),
		rgb(0.99679, 0.99796, 0.86019),
		rgb(0.99715, 0.99818, 0.87218),
		rgb(0.99749, 0.99839, 0.88411),
		rgb(0.99781, 0.99859, 0.89598),
		rgb(0.99811, 0.99878, 0.90779),
		rgb(0.99839, 0.99896, 0.91954),
		rgb(0.99866, 0.99913, 0.93123),
		rgb(0.99891, 0.99929, 0.94285),
		rgb(0.99915, 0.99944, 0.95441),
		rgb(0.99938, 0.99959, 0.96591),
		rgb(0.99959, 0.99973, 0.97734),
		rgb(0.99966, 0.99977, 0.98907),
		rgb(1.00000, 1.00000, 1.00000)
	)
	#return desired palette
	return (color.palette[seq.int(ratio.to.n(min), ratio.to.n(max), length.out=n)])
}

#creates a barplot to show an example of the palette
show.sample.ccs.fireprint.palette <- function(n=256, min=0, max=1) {
	#save old palette to be restored after showing the example
	old.palette <- palette()
	#change palette to ccs palette
	palette(get.ccs.fireprint.palette(n, min, max))
	#plot the example
	barplot(rep(1, n), col=seq(1,length(palette()), by=length(palette()) / n), space=0, border=NA)
	#set title
	title(main=paste('get.ccs.fireprint.palette(n=', n, ', min=', min, ', max=', max, ')', sep=''))
	#restore previous palette
	palette(old.palette)
}
