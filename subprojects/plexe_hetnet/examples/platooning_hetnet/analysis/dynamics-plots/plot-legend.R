library('tikzDevice')

source('plot-utils.R')
source('ccs.palettes.R')

source('set-colors.R')

plot.legend <- function(outputFile, outputDir = './') {
    plot.width <- 308.43/72/1.7
    plot.height <- 184.546607/72/9
    plot.margin <- c(0, 0, 0, 0)
    leg.inset <- c(0, 0, 0, 0)
    done <- myps(outputFile = outputFile, width=plot.width, height=plot.height)
    par(mar=plot.margin, xpd=T)
    plot.new()
    plot.window(xlim=c(0, 1000), ylim=c(0, 1), yaxs="i", xaxs="i")

    outline <- T
    width <- 2
    outcex <- .25
    outlwd <- .25

    # make legend horizontal on two lines (ncols runs vertically)
    sq <- c(1, 5, 2, 6, 3, 7, 4, 8)
    legend(
        "top",
        paste0("$V_", 0:7, "$")[sq],
        bty="n",
        col=(1:8)[sq],
        pch=NA,
        lty=(1:8)[sq],
        ncol=4,
        lwd=2,
        box.lwd=0,
        pt.bg=NA,
        pt.cex=NA,
        cex=.6,
        seg.len=c(4, 4, 4, 4, 3.6, 4, 4, 4)[sq],
        text.width=strwidth('abc')
    )

    done()

}
plot.events.legend <- function(outputFile, outputDir = './') {
    plot.width <- 308.43/72/1.45
    plot.height <- 184.546607/72/12
    plot.margin <- c(0, 0, 0, 0)
    leg.inset <- c(0, 0, 0, 0)
    done <- myps(outputFile = outputFile, width=plot.width, height=plot.height)
    par(mar=plot.margin, xpd=T)
    plot.new()
    plot.window(xlim=c(0, 1000), ylim=c(0, 1), yaxs="i", xaxs="i")

    outline <- T
    width <- 2
    outcex <- .25
    outlwd <- .25

    legend(
        "top",
        c("failure", "recovery", "braking", "handover"),
        bty="n",
        col=c(rep('black', 3), rgb(0.7, 0.7, 0.7, 0.5)),
        pch=c(4:3, NA, NA),
        lty=c(NA, NA, 2, 1),
        ncol=4,
        lwd=c(rep(2, 3), 10),
        box.lwd=0,
        pt.bg=NA,
        pt.cex=.8,
        cex=.6,
        seg.len=4
        #text.width=strwidth('abcde')
    )
    done()

}
plot.events.handover <- function(outputFile, outputDir = './') {
    done <- myps(outputFile = outputFile, width=plot.width, height=plot.height)
    par(mar=plot.margin, xpd=T)
    plot.new()
    plot.window(xlim=c(0, 1000), ylim=c(0, 1), yaxs="i", xaxs="i")

    outline <- T
    width <- 2
    outcex <- .25
    outlwd <- .25

    legend(
        "top",
        c("failure", "recovery", "handover"),
        bty="n",
        col='black',
        pch=c(4:3, NA),
        lty=c(NA, NA, 2),
        ncol=3,
        lwd=2,
        box.lwd=0,
        pt.bg=NA,
        pt.cex=1,
        cex=.6,
        seg.len=3,
        text.width=strwidth('abcde')
    )
    done()

}

plot.legend(outputFile='legend-vehicles')
plot.events.legend(outputFile='legend-events')
plot.events.handover(outputFile='legend-handover')
