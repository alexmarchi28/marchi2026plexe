# To obtain the data for this plot go to the following folders:
# - examples/platooning (11p only)
# - subprojects/plexe_vlc/examples/platooning_vlc (vlc only)
# - subprojects/plexe_lte/examples/platooning_lte (11p + lte)
# - subprojects/plexe_hetnet/examples/platooning_hetnet (11p + vlc + lte)
# In each folder run the measure-timings.sh script which launches 3 simulations with a single platoon made of 8, 16, and 32 cars
# for each simulation the script measures the time required to run the simulation and writes it into scalability-timings.txt
# take the values from there and copy them here
# results here are for Michele's mac book pro 18 (i9) and will give different results for different machines

library('tikzDevice')
library(RColorBrewer)
library(plyr)

source('dynamics-plots/plot-utils.R')
source('dynamics-plots/ccs.palettes.R')

palette(get.ccs.fireprint.palette(n=4, min=0, max=0.7))

simulation.time <- 240
only11p <- data.frame(id=0, intf="802.11p only", nCars = c(8, 16, 32), time=c(18.851, 54.895, 181.666))
onlyvlc <- data.frame(id=1, intf="VLC only", nCars = c(8, 16, 32), time=c(30.495, 171.662, 22*60+34.435))
lte11p <- data.frame(id=2, intf="802.11p and C-V2X", nCars = c(8, 16, 32), time=c(76.330, 188.407, 9*60+37.132))
allint <- data.frame(id=3, intf="802.11p, C-V2X, and VLC", nCars = c(8, 16, 32), time=c(137.047, 9*60+7.837, 49*60+26.646))
all.data <- rbind(only11p, onlyvlc, lte11p, allint)

#graphs params
plot.width <- 308.43/72
plot.height <- 184.546607/72/1.25
plot.margin <- c(3.1, 3.3, 0.4, 0.4)
leg.inset <- c(0, 0, 0, 0)


plot.graph <- function(outputFile, xlims, ylims, d) {
    done <- myps(outputFile = outputFile, width=plot.width, height=plot.height)
    par(mar=plot.margin, xpd=F)
    plot.new()
    plot.window(xlim=xlims, ylim=ylims, yaxs="i", xaxs="i")

    abline(h=1, col='gray', lty=1, lwd=2)
    ddply(d, .(intf), function(x) {
        x <- x[order(x$nCars),]
        lines(x$nCars, x$time/240, col=x$id+1, lty=x$id+1, lwd=2)
        points(x$nCars, x$time/240, col=x$id+1, pch=x$id+1, lwd=2)
    })

    legend(
        "topleft",
        unique(d$intf),
        bty="n",
        col=unique(d$id)+1,
        pch=unique(d$id)+1,
        lty=unique(d$id)+1,
        ncol=1,
        lwd=2,
        box.lwd=0,
        pt.bg=NA,
        pt.cex=1,
        cex=.6,
        seg.len=3,
        inset=c(0, 0)
    )
    axis(1, lwd=0, lwd.ticks=1, las=1)
    axis(2, lwd=0, lwd.ticks=1, las=1)
    title(ylab="real time factor [\\#]", line=2.2)
    title(xlab="number of vehicles [\\#]", line=2)

    box()
    done()
}

plot.graph("scalability", c(5, 37), c(0, 14), all.data)
plot.graph("scalability-zoom", c(5, 37), c(0, 6), all.data)
