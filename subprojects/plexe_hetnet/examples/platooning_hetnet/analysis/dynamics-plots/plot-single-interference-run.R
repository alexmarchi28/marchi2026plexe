#!/usr/bin/env Rscript

library("tikzDevice")
library("plyr")

args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 3) {
    stop("usage: plot-single-interference-run.R <input-dir> <output-dir> <run-id>")
}

script_arg <- grep("^--file=", commandArgs(), value = TRUE)
script_path <- normalizePath(sub("^--file=", "", script_arg))
script_dir <- dirname(script_path)
setwd(script_dir)

source("plot-utils.R")
source("ccs.palettes.R")
source("set-colors.R")

input_dir <- normalizePath(args[1], mustWork = TRUE)
output_dir <- args[2]
run_id <- as.integer(args[3])

if (is.na(run_id)) {
    stop("run-id must be numeric")
}

dir.create(output_dir, recursive = TRUE, showWarnings = FALSE)
output_dir <- paste0(normalizePath(output_dir, mustWork = TRUE), "/")

plot.width <- 308.43 / 72
plot.height <- 184.546607 / 72 / 1.25 * 0.8
plot.margin.in <- c(0.5147364, 0.5479452, 0.0166044, 0.0166044)

serial.find.value <- function(dynamics, time, vehicle, field) {
    dynamics <- subset(dynamics, nodeId == vehicle)
    dynamics[abs(dynamics$time - time) == min(abs(dynamics$time - time)), ][[field]][1]
}

find.value <- Vectorize(serial.find.value, vectorize.args = c("time", "vehicle"))

plot.graph <- function(output_file, dynamics, events, field, xlimv, ylimv, yaxis, handovers = NA) {
    done <- myps(outputFile = output_file, width = plot.width, height = plot.height, outputDir = output_dir)
    par(mai = plot.margin.in, xpd = FALSE)
    plot.new()
    plot.window(xlim = xlimv, ylim = ylimv, yaxs = "i", xaxs = "i")

    if (!all(is.na(handovers))) {
        l.ho <- subset(handovers, handoverStart == 1 & handoverId == 0)
        for (tm in l.ho$time) {
            ho.event <- subset(handovers, time > tm - 10 & time < tm + 10)
            ho.start <- min(ho.event$time)
            ho.end <- max(ho.event$time)
            rect(
                xleft = ho.start,
                xright = ho.end,
                ybottom = ylimv[1],
                ytop = ylimv[2],
                border = NA,
                col = rgb(0.7, 0.7, 0.7, 0.5)
            )
        }
    }

    ddply(dynamics, .(nodeId), function(x) {
        x <- x[order(x$time), ]
        id <- x[1, ]$nodeId
        if (id == 0 && field == "distance") return(0)
        lines(x$time, x[[field]], col = id + 1, lty = id + 1, lwd = 2)
    })

    braking <- subset(events, eventFailure == 2)
    events <- subset(events, eventFailure %in% c(0, 1))
    if (nrow(events) > 0) {
        points(
            events$time,
            find.value(dynamics, events$time, events$eventVehicleId, field),
            pch = events$eventFailure + 3,
            col = events$eventVehicleId + 1,
            lwd = 2,
            cex = 1.5
        )
    }
    if (nrow(braking) > 0) {
        abline(v = braking$time, col = "black", lwd = 2, lty = 2)
    }

    axis(1, lwd = 0, lwd.ticks = 1, las = 1)
    axis(2, lwd = 0, lwd.ticks = 1, las = 1)
    title(ylab = yaxis, line = 2.2)
    title(xlab = "time [s]", line = 2)
    box()
    done()
}

plot.fer <- function(output_file, fer, field, yaxis, handovers = NA) {
    done <- myps(outputFile = output_file, width = 308.43 / 72, height = 184.546607 / 72 / 1.25, outputDir = output_dir)
    par(mai = plot.margin.in, xpd = FALSE)
    plot.new()
    plot.window(xlim = c(0, 240), ylim = c(0, 11.2), yaxs = "i", xaxs = "i")

    tech <- c("11p", "LTE", "VLC")
    transf <- function(x, t) {
        if (t == tech[1]) return(x * 3)
        if (t == tech[2]) return(x * 3 + 4)
        x * 3 + 8
    }

    ddply(fer, .(statsId), function(x) {
        x <- x[order(x$time), ]
        id <- x[1, ]$statsId
        for (t in tech) {
            lines(x$time, transf(x[[paste0(field, t)]], t), col = id + 1, lty = id + 1, lwd = 2)
        }
    })

    if (!all(is.na(handovers))) {
        l.ho <- subset(handovers, handoverStart == 1 & handoverId == 0)
        for (tm in l.ho$time) {
            ho.event <- subset(handovers, time > tm - 10 & time < tm + 10)
            ho.start <- min(ho.event$time)
            ho.end <- max(ho.event$time)
            for (t in tech) {
                rect(
                    xleft = ho.start,
                    xright = ho.end,
                    ybottom = transf(0, t),
                    ytop = transf(1, t),
                    border = NA,
                    col = rgb(0.7, 0.7, 0.7, 0.5)
                )
            }
        }
    }

    axis(1, lwd = 0, lwd.ticks = 1, las = 1)
    axis(2, lwd = 0, lwd.ticks = 1, las = 1, at = c(4 * 0:2, 4 * 0:2 + 3), labels = c(rep(0, 3), rep(1, 3)))
    title(ylab = yaxis, line = 2.2)
    title(xlab = "time [s]", line = 2)
    par(xpd = TRUE)
    rect(0, 0, 240, 3)
    rect(0, 4, 240, 7)
    rect(0, 8, 240, 11)
    text(-4, 1.5, labels = "11p", srt = 90, cex = 0.8)
    text(-4, 5.5, labels = "C-V2X", srt = 90, cex = 0.8)
    text(-4, 9.5, labels = "VLC", srt = 90, cex = 0.8)
    done()
}

load(file.path(input_dir, "InterferenceScenarioDynamics.Rdata"))
dynamics <- subset(allData, runNumber == run_id)
if (nrow(dynamics) == 0) {
    stop("no dynamics data found for run ", run_id)
}
dynamics$speed <- dynamics$speed * 3.6
scenario_id <- unique(dynamics$scenario)
temp_leader <- unique(dynamics$useTempLeader)
if (length(scenario_id) != 1 || length(temp_leader) != 1) {
    stop("expected exactly one scenario and one temp leader value")
}

load(file.path(input_dir, "InterferenceScenarioEvents.Rdata"))
events <- subset(allData, runNumber == run_id)
load(file.path(input_dir, "InterferenceScenarioHandovers.Rdata"))
handovers <- subset(allData, runNumber == run_id)
load(file.path(input_dir, "InterferenceScenarioFER.Rdata"))
fer <- subset(allData, runNumber == run_id)

plot.graph(
    paste0("interference-distance-run-", run_id),
    dynamics,
    events,
    "distance",
    c(0, 240),
    c(0, 37),
    "distance [m]",
    handovers
)

plot.graph(
    paste0("interference-acceleration-run-", run_id),
    dynamics,
    events,
    "acceleration",
    c(0, 240),
    c(-3, 2.2),
    "acceleration [m/s$^2$]",
    handovers
)

plot.fer(
    paste0("interference-leader-fer-run-", run_id),
    fer,
    "leaderFer",
    "FER",
    handovers
)

plot.fer(
    paste0("interference-front-fer-run-", run_id),
    fer,
    "frontFer",
    "FER",
    handovers
)
