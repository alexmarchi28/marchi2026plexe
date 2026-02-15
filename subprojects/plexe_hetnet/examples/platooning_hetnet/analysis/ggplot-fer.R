library(ggplot2)
library(reshape2)
library(plyr)

load('../results/HetNetHandover.Rdata')
handovers <- allData
load("../results/HetNet.Rdata")

allData$leaderDelayLTE[allData$leaderDelayLTE < 0] <- NA
allData$leaderInterarrivalLTE[allData$leaderInterarrivalLTE < 0] <- NA
allData$frontDelayLTE[allData$frontDelayLTE < 0] <- NA
allData$frontInterarrivalLTE[allData$frontInterarrivalLTE < 0] <- NA
fields <- c(
    "time",
    "statsId",
    "controller",
    "headway",
    "runNumber",
    "frontDelay11p",
    "frontDelayLTE",
    "frontDelayVLC",
    "frontFer11p",
    "frontFerLTE",
    "frontFerVLC",
    "frontInterarrival11p",
    "frontInterarrivalLTE",
    "frontInterarrivalVLC",
    "leaderDelay11p",
    "leaderDelayLTE",
    "leaderDelayVLC",
    "leaderFer11p",
    "leaderFerLTE",
    "leaderFerVLC",
    "leaderInterarrival11p",
    "leaderInterarrivalLTE",
    "leaderInterarrivalVLC"
)
ids <- c(
    "time",
    "statsId",
    "controller",
    "headway",
    "runNumber"
)

split.field <- function(x) {
    l <- ifelse(grepl("front", x, fixed = T), "front", "leader")
    m <- ifelse(grepl("Delay", x, fixed = T), "Delay", ifelse(grepl("Fer", x, fixed = T), "Fer", "Interarrival"))
    t <- ifelse(grepl("11p", x, fixed = T), "11p", ifelse(grepl("VLC", x, fixed = T), "VLC", "LTE"))
    return(data.frame(metric=paste0(l, m), technology=t, stringsAsFactors=F))
}

get.plot <- function(d, field, leader, ho, run=0) {
    d <- subset(d, runNumber==run)
    ho <- subset(ho, runNumber==run)
    prefix <- ifelse(leader, paste0("leader", field), paste0("front", field))
    f11p <- paste0(prefix, "11p")
    fVLC <- paste0(prefix, "VLC")
    fLTE <- paste0(prefix, "LTE")
    p <- ggplot(d, aes_string(x="time", y=prefix, color="factor(statsId)", linetype="technology")) + geom_line() + geom_point(size=1.5, aes(shape=technology)) + facet_grid(statsId~.)
    ho$statsId <- ho$handoverId
    p <- p + geom_vline(data=ho, aes(xintercept=time), color=rgb(.2, .2, .2, .2))
    return(p)
}

molten <- melt(allData, id.vars=ids)
molten <- cbind(molten, split.field(molten$variable))
molten$variable <- NULL
d <- reshape(molten, idvar = c(ids, "technology"), timevar = "metric", direction = "wide", times="")
names(d) <- gsub("value.", "", names(d))

leaderFer <- get.plot(d, "Fer", T, handovers)
print(leaderFer)

frontFer <- get.plot(d, "Fer", F, handovers)
print(frontFer)

leaderDelay <- get.plot(d, "Delay", T, handovers)
print(leaderDelay)

frontDelay <- get.plot(d, "Delay", F, handovers)
print(frontDelay)

leaderInterarrival <- get.plot(d, "Interarrival", T, handovers)
print(leaderInterarrival)

frontInterarrival <- get.plot(d, "Interarrival", F, handovers)
print(frontInterarrival)

