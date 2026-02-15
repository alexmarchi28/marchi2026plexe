library('tikzDevice')
library(RColorBrewer)
library(plyr)

source('plot-utils.R')

source('ccs.palettes.R')

source('set-colors.R')

#graphs params
plot.width <- 308.43/72
plot.height <- 184.546607/72/1.25
plot.margin <- c(3.1, 3.3, 0.1, 0.1)#/6.0225
plot.margin.in <- c(0.5147364, 0.5479452, 0.0166044, 0.0166044)
leg.inset <- c(0, 0, 0, 0)

plot.fer <- function(outputFile, fer, field, yaxis, handovers=NA, xvalues=T) {

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
    plot.window(xlim=c(0, 240), ylim=c(0, 11.2), yaxs="i", xaxs="i")


    tech <- c("11p", "LTE", "VLC")
    transf <- function(x, t) {
        if (t == tech[1]) return(x*3)
        if (t == tech[2]) return(x*3+4)
        if (t == tech[3]) return(x*3+8)
    }
    ddply(fer, .(statsId), function(x) {
        x <- x[order(x$time),]
        id <- x[1,]$statsId
        for(t in tech) {
            lines(x$time, transf(x[[paste0(field, t)]], t), , col=id+1, lty=id+1, lwd=2)
        }
    })
    if (!all(is.na(handovers))) {
        l.ho <- subset(handovers, handoverStart == 1 & handoverId == 0)
        for (tm in l.ho$time) {
            ho.event <- subset(handovers, time > tm - 10 & time < tm + 10)
            ho.start <- min(ho.event$time)
            ho.end <- max(ho.event$time)
            for(t in tech) {
                rect(xleft=ho.start, xright=ho.end, ybottom=transf(0, t), ytop=transf(1, t), border=NA, col=rgb(0.7, 0.7, 0.7, 0.5))
            }
        }
    }

    if (xvalues) axis(1, lwd=0, lwd.ticks=1, las=1)
    else axis(1, lwd=0, lwd.ticks=1, las=1, labels=NA)
    axis(2, lwd=0, lwd.ticks=1, las=1, at=c(4*0:2, 4*0:2+3), labels=c(rep(0, 3), rep(1, 3)))
    title(ylab=yaxis, line=2.2)
    if (xvalues) title(xlab="time [s]", line=2)
    par(xpd=T)
    rect(0, 0, 240, 3)
    rect(0, 4, 240, 7)
    rect(0, 8, 240, 11)
    text(-4, 1.5, labels="11p", srt=90, cex=.8)
    text(-4, 5.5, labels="C-V2X", srt=90, cex=.8)
    text(-4, 9.5, labels="VLC", srt=90, cex=.8)
    done()
}

plot.fers <- function(config, fer, handovers=NA) {
    for (n in unique(fer$runNumber)) {
        f <- subset(fer, runNumber == n)
        if (!all(is.na(handovers))) ho <- subset(handovers, runNumber == n)
        else ho <- NA
        for (xvalues in c(T, F)) {
            l.name <- paste(config, "leader-fer", n, sep="-")
            if (!xvalues) l.name <- paste(l.name, "noaxis", sep="-")
            f.name <- paste(config, "front-fer", n, sep="-")
            if (!xvalues) f.name <- paste(f.name, "noaxis", sep="-")
            plot.fer(l.name, f, "leaderFer", "FER", ho, xvalues)
            plot.fer(f.name, f, "frontFer", "FER", ho, xvalues)
        }
    }
}

load('../../results/InterferenceScenarioHandovers.Rdata')
handovers <- allData
load('../../results/InterferenceScenarioFER.Rdata')
fer <- allData
plot.fers("interference", fer, handovers)
