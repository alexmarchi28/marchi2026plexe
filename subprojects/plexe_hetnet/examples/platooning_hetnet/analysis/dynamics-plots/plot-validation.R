library('tikzDevice')
library(RColorBrewer)
library(plyr)

source('plot-utils.R')

source('ccs.palettes.R')

source('set-colors.R')

#graphs params
plot.width <- 308.43/72
plot.height <- 184.546607/72/1.25/1.5
plot.margin <- c(3.1, 3.3, 0.1, 0.1)
plot.margin.in <- c(0.5147364, 0.5479452, 0.0166044, 0.0166044)
leg.inset <- c(0, 0, 0, 0)

plot.validation <- function(outputFile, dynamics, field, xlims, ylims, yaxis, handovers=NA, lims.selector=NA, xvalues=T) {

    pm <- plot.margin.in
    ph <- plot.height
    rh <- 0.4
    if (!xvalues) {
        pm[1] <- pm[1] - rh
        ph <- ph - rh
    }
    done <- myps(outputFile = outputFile, width=plot.width, height=ph)
    par(mai=pm, xpd=F)
    plot.new()
    plot.window(xlim=xlims[[field]][[lims.selector]], ylim=ylims[[field]][[lims.selector]], yaxs="i", xaxs="i")

    if (!all(is.na(handovers))) {
        handovers <- subset(handovers, handoverStart == 1)
        abline(v=handovers$time, col=handovers$handoverId+1, lwd=2, lty=2)
    }
    ddply(dynamics, .(nodeId), function(x) {
        x <- x[order(x$time),]
        id <- x[1,]$nodeId
        if (id == 0 & field == "distance") return(0)
        lines(x$time, x[[field]], col=id+1, lty=id+1, lwd=2)
    })

    axis(1, lwd=0, lwd.ticks=1, las=1)
    axis(2, lwd=0, lwd.ticks=1, las=1)
    title(ylab=yaxis, line=2.2)
    title(xlab="time [s]", line=2)
    box()
    done()
}

default.xlims <- c(100, 128)
default.dist.ylims <- c(14, 18.3)
default.acc.ylims <- c(-4, 3.2)

xlims <- list()
for (i in 1:3) xlims[["distance"]][[i]] <- default.xlims
for (i in 1:3) xlims[["acceleration"]][[i]] <- default.xlims

ylims <- list()
for (i in 1:2) ylims[["distance"]][[i]] <- default.dist.ylims
ylims[["distance"]][[3]] <- c(10, 18.5)
for (i in 1:3) ylims[["acceleration"]][[i]] <- default.acc.ylims

plot.validations <- function(config, dynamics, handovers=NA) {
    for (n in unique(dynamics$runNumber)) {
        for (i in unique(dynamics$interface)) {
            for (xvalues in c(F, T)) {
                dyn <- subset(dynamics, interface == i & runNumber == n)

                d.name <- paste(config, "distance", i, n, sep="-")
                if (!xvalues) d.name <- paste(d.name, "noaxis", sep="-")

                a.name <- paste(config, "acceleration", i, n, sep="-")
                if (!xvalues) a.name <- paste(a.name, "noaxis", sep="-")

                plot.validation(d.name, dyn, "distance", xlims, ylims, "distance [m]", lims.selector=i+1, xvalues=xvalues)
                plot.validation(a.name, dyn, "acceleration", xlims, ylims, "accel. [m/s$^2$]", lims.selector=i+1, xvalues=xvalues)
            }
        }
    }
}

load('../../results/ValidationDynamics.Rdata')
dynamics <- allData
dynamics$speed <- dynamics$speed * 3.6
plot.validations("validation", dynamics)
