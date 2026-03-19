library('tikzDevice')
library(RColorBrewer)
library(plyr)

source('plot-utils.R')

source('ccs.palettes.R')

source('set-colors.R')

#graphs params
plot.width <- 308.43/72
plot.height <- 184.546607/72/1.25*0.8
plot.margin <- c(3.1, 3.3, 0.1, 0.1)#/6.0225
plot.margin.in <- c(0.5147364, 0.5479452, 0.0166044, 0.0166044)
leg.inset <- c(0, 0, 0, 0)

serial.find.value <- function(dynamics, time, vehicle, field) {
    dynamics <- subset(dynamics, nodeId == vehicle)
    dynamics[abs(dynamics$time - time) == min(abs(dynamics$time - time)),][[field]][1]
}
find.value <- Vectorize(serial.find.value, vectorize.args=c("time", "vehicle"))

plot.graph <- function(outputFile, dynamics, events, field, xlims, ylims, scenario, yaxis, handovers=NA, xvalues=T) {

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
    yl <- ylims[[toString(scenario)]][[field]]
    plot.window(xlim=xlims[[toString(scenario)]][[field]], ylim=yl, yaxs="i", xaxs="i")

    if (!all(is.na(handovers))) {
        l.ho <- subset(handovers, handoverStart == 1 & handoverId == 0)
        for (tm in l.ho$time) {
            ho.event <- subset(handovers, time > tm - 10 & time < tm + 10)
            ho.start <- min(ho.event$time)
            ho.end <- max(ho.event$time)
            rect(xleft=ho.start, xright=ho.end, ybottom=yl[1], ytop=yl[2], border=NA, col=rgb(0.7, 0.7, 0.7, 0.5))
        }
    }
    ddply(dynamics, .(nodeId), function(x) {
        x <- x[order(x$time),]
        id <- x[1,]$nodeId
        if (id == 0 & field == "distance") return(0)
        lines(x$time, x[[field]], col=id+1, lty=id+1, lwd=2)
    })
    braking <- subset(events, eventFailure == 2)
    events <- subset(events, eventFailure %in% c(0, 1))
    points(events$time, find.value(dynamics, events$time, events$eventVehicleId, field), pch=events$eventFailure+3, col=events$eventVehicleId+1, lwd=2, cex=1.5)
    abline(v=braking$time, col="black", lwd=2, lty=2)

    #legend(
    #    "topright",
    #    paste0("$V_", 0:7, "$"),
    #    bty="n",
    #    col=1:8,
    #    pch=NA,
    #    lty=1:8,
    #    ncol=2,
    #    lwd=2,
    #    box.lwd=0,
    #    pt.bg=NA,
    #    pt.cex=NA,
    #    cex=.6,
    #    seg.len=3,
    #    inset=c(0, 0)
    #)
    #legend(
    #    "topleft",
    #    c("failure", "recovery", "braking event"),
    #    bty="n",
    #    col='black',
    #    pch=c(4:3, NA),
    #    lty=c(NA, NA, 2),
    #    ncol=1,
    #    lwd=2,
    #    box.lwd=0,
    #    pt.bg=NA,
    #    pt.cex=1,
    #    cex=.6,
    #    seg.len=3,
    #    inset=c(0, 0)
    #)

    if (xvalues) axis(1, lwd=0, lwd.ticks=1, las=1)
    else axis(1, lwd=0, lwd.ticks=1, las=1, labels=NA)
    axis(2, lwd=0, lwd.ticks=1, las=1)
    title(ylab=yaxis, line=2.2)
    if (xvalues) title(xlab="time [s]", line=2)
    box()
    done()
}

# mapping of scenarios and fields to x and y lims
xlims <- list(
    "0" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "1" = list(
        distance = c(0, 125),
        acceleration = c(0, 125)
    ),
    "2" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "3" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "4" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "5" = list(
        distance = c(0, 125),
        acceleration = c(0, 125)
    ),
    "6" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "7" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "8" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "9" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    ),
    "10" = list(
        distance = c(0, 240),
        acceleration = c(0, 240)
    )
)
ylims <- list(
    "0" = list(
        distance = c(0, 37),
        acceleration = c(-3, 2.2)
    ),
    "1" = list(
        distance = c(0, 37),
        acceleration = c(-3, 2.2)
    ),
    "2" = list(
        distance = c(0, 15),
        acceleration = c(-9, 3.2)
    ),
    "3" = list(
        distance = c(0, 15),
        acceleration = c(-9, 3.2)
    ),
    "4" = list(
        distance = c(0, 37),
        acceleration = c(-3, 2.2)
    ),
    "5" = list(
        distance = c(0, 37),
        acceleration = c(-3, 2.2)
    ),
    "6" = list(
        distance = c(0, 23),
        acceleration = c(-9, 3.2)
    ),
    "7" = list(
        distance = c(0, 23),
        acceleration = c(-9, 3.2)
    ),
    "8" = list(
        distance = c(0, 37),
        acceleration = c(-3, 2.2)
    ),
    "9" = list(
        distance = c(0, 23),
        acceleration = c(-9, 3.2)
    ),
    "10" = list(
        distance = c(0, 23),
        acceleration = c(-9, 3.2)
    )
)

plot.dynamics <- function(config, dynamics, events, handovers=NA) {
    for (n in unique(dynamics$runNumber)) {
        for (i in unique(dynamics$scenario)) {
            for (j in unique(dynamics$useTempLeader)) {
                for (xvalues in c(F, T)) {
                    dyn <- subset(dynamics, scenario == i & useTempLeader == j & runNumber == n)
                    ev <- subset(events, scenario == i & useTempLeader == j & runNumber == n)
                    if (nrow(dyn) == 0) next
                    if (!all(is.na(handovers))) ho <- subset(handovers, runNumber == n)
                    else ho <- NA

                    d.name <- paste(config, "distance", i, j, sep="-")
                    if (length(unique(dynamics$runNumber)) > 1) d.name <- paste(d.name, n, sep="-")
                    if (!xvalues) d.name <- paste(d.name, "noaxis", sep="-")

                    a.name <- paste(config, "acceleration", i, j, sep="-")
                    if (length(unique(dynamics$runNumber)) > 1) a.name <- paste(a.name, n, sep="-")
                    if (!xvalues) a.name <- paste(a.name, "noaxis", sep="-")

                    plot.graph(d.name, dyn, ev, "distance", xlims, ylims, i, "distance [m]", ho, xvalues)
                    plot.graph(a.name, dyn, ev, "acceleration", xlims, ylims, i, "acceleration [m/s$^2$]", ho, xvalues)
                }
            }
        }
    }
}

load('../../results/ArtificialFailuresEvents.Rdata')
events <- allData
load('../../results/ArtificialFailuresDynamics.Rdata')
dynamics <- allData
dynamics$speed <- dynamics$speed * 3.6
plot.dynamics("artificial", dynamics, events)

load('../../results/ACCOnlyEvents.Rdata')
events <- allData
load('../../results/ACCOnlyDynamics.Rdata')
dynamics <- allData
dynamics$speed <- dynamics$speed * 3.6
plot.dynamics("acconly", dynamics, events)

load('../../results/MultipleArtificialFailuresEvents.Rdata')
events <- allData
load('../../results/MultipleArtificialFailuresDynamics.Rdata')
dynamics <- allData
dynamics$speed <- dynamics$speed * 3.6
plot.dynamics("multiple", dynamics, events)

load('../../results/SimultaneousArtificialFailuresEvents.Rdata')
events <- allData
load('../../results/SimultaneousArtificialFailuresDynamics.Rdata')
dynamics <- allData
dynamics$speed <- dynamics$speed * 3.6
plot.dynamics("simultaneous", dynamics, events)

load('../../results/InterferenceScenarioEvents.Rdata')
events <- allData
load('../../results/InterferenceScenarioDynamics.Rdata')
dynamics <- allData
load('../../results/InterferenceScenarioHandovers.Rdata')
handovers <- allData
dynamics$speed <- dynamics$speed * 3.6
plot.dynamics("interference", dynamics, events, handovers)
