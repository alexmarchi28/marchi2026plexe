#!/usr/bin/env Rscript
#
# Copyright (C) 2016-2019 Bastian Bloessl <bloessl@ccs-labs.org>
# Copyright (C) 2016-2019 Michele Segata <segata@ccs-labs.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

source('./omnet_helpers.R')
source('./generic-parsing-util.R')

args <- commandArgs(trailingOnly = T)

run.scavetool <- function(args, quiet=F) {
    # system2() uses a shell command line; quote each argument to preserve
    # filter expressions that contain spaces/parentheses.
    out <- system2("opp_scavetool", args=shQuote(args), stdout=T, stderr=T)
    rc <- attr(out, "status")
    if (is.null(rc)) rc <- 0
    if (rc != 0) {
        stop(paste("opp_scavetool failed with exit code", rc, "for args:", paste(args, collapse=" ")))
    }
    if (!quiet) {
        cat(paste(out, collapse="\n"), "\n")
    }
    return(out)
}

safe.as.numeric <- function(x) {
    suppressWarnings(as.numeric(x))
}

# OMNeT++ 6 writes vector/index format version 3, unsupported by the legacy
# R package `omnetpp`. This fallback loader uses opp_scavetool and recreates
# the same wide table expected by the downstream code.
prepare.vector.scavetool <- function(vecFile, mapConfig) {
    if (Sys.which("opp_scavetool") == "") {
        stop("legacy parser failed and opp_scavetool is not available in PATH")
    }

    requested <- gsub("^name\\(|\\)$", "", mapConfig$names)
    requested <- unique(requested)

    # Match both "name" and "name:vector" to handle both scalar-like and
    # vector-like signal names in result files.
    terms <- unlist(lapply(requested, function(n) c(
        paste0("name =~ \"", n, "\""),
        paste0("name =~ \"", n, ":vector\"")
    )))
    filter <- paste0("(", paste(terms, collapse=" OR "), ")")

    qargs <- c("q", "-T", "v", "-l", "--tabs", "-b", "-f", filter, vecFile)
    qout <- run.scavetool(qargs, quiet=T)
    qrows <- qout[startsWith(qout, "vector\t")]
    if (length(qrows) == 0) {
        empty <- data.frame(time=numeric(0))
        for (n in requested) {
            empty[[gsub(":vector$", "", n)]] <- numeric(0)
        }
        return(empty)
    }

    parsed <- lapply(qrows, function(line) {
        f <- strsplit(line, "\t", fixed=T)[[1]]
        data.frame(module=f[2], name=f[3], stringsAsFactors=F)
    })
    meta <- do.call(rbind, parsed)

    tmp <- tempfile(pattern="plexe_scavetool_", fileext=".csv")
    on.exit(unlink(tmp), add=T)
    xargs <- c(
        "x",
        "-T", "v",
        "-F", "CSV-S",
        "-x", "vectorLayout=vertical",
        "-x", "columnNames=false",
        "-o", tmp,
        "-f", filter,
        vecFile
    )
    run.scavetool(xargs, quiet=T)

    raw <- data.table::fread(tmp, header=F, na.strings=c("", "NA"), fill=T, showProgress=F)
    if (nrow(raw) == 0 || ncol(raw) == 0) {
        empty <- data.frame(time=numeric(0))
        for (n in requested) {
            empty[[gsub(":vector$", "", n)]] <- numeric(0)
        }
        return(empty)
    }

    per.module <- list()
    for (i in seq_len(nrow(meta))) {
        c.time <- 2 * i - 1
        c.val <- 2 * i
        if (c.val > ncol(raw)) break

        d <- data.frame(
            time=safe.as.numeric(raw[[c.time]]),
            value=safe.as.numeric(raw[[c.val]])
        )
        d <- subset(d, !is.na(time) & !is.na(value))
        if (nrow(d) == 0) next

        vec.name <- meta$name[i]
        # Canonicalize vector-style names to match map-config expectations.
        vec.name <- gsub(":vector$", "", vec.name)
        colnames(d)[2] <- vec.name

        mod <- meta$module[i]
        if (is.null(per.module[[mod]])) {
            per.module[[mod]] <- d
        } else {
            per.module[[mod]] <- merge(per.module[[mod]], d, by="time", all=T)
        }
    }

    if (length(per.module) == 0) {
        empty <- data.frame(time=numeric(0))
        for (n in requested) {
            empty[[gsub(":vector$", "", n)]] <- numeric(0)
        }
        return(empty)
    }

    merged <- as.data.frame(data.table::rbindlist(per.module, use.names=T, fill=T))
    rownames(merged) <- NULL

    # Ensure all requested columns exist so downstream subset conditions do not fail.
    for (n in requested) {
        canon <- gsub(":vector$", "", n)
        if (!(canon %in% names(merged))) {
            merged[[canon]] <- NA_real_
        }
    }

    return(merged)
}

#parameters of the script:
#1: input vector file
#2: map config file
#3: map config configuration
#4: output file prefix

if (length(args) < 4) {
    stop("generic-parse.R requires at least 4 parameters")
}
infile  <- args[1]
mapfile <- args[2]
config  <- args[3]
prefix  <- args[4]
outtype <- "Rdata"
if (length(args) == 5) {
    outtype  <- args[5]
}

outfile <- paste(dirname(infile), '/', prefix, '.', basename(infile), sep='')
outfile <- gsub(".vec", paste(".", outtype, sep=''), outfile)

#load map file
map <- parse.map(mapfile)
#check whether required config exists
if (is.null(map[[config]])) {
    stop("required config", config, "does not exist in", mapfile)
}
#get simulation parameters
params <- get.params(infile, map[[config]]$fields)

# debug output
cat("infile: ", infile, "\n")
cat("outfile; ", outfile, "\n")
cat("-------------------------\n")
cat("run: ", params$runNumber, "\n")

# wastes a lot of storage - but it's easy to handle
selector <- get.selector(map[[config]]$module, map[[config]]$names)
condition <- get.subset.condition(map[[config]]$names)
toclean <- tryCatch({
    d <- prepare.vector(infile, selector)
    names(d) <- rename.columns(names(d))
    d
}, error=function(e) {
    msg <- conditionMessage(e)
    cat("legacy parser failed:", msg, "\n")
    cat("trying OMNeT++ 6 compatible parser based on opp_scavetool...\n")
    prepare.vector.scavetool(infile, map[[config]])
})
toclean <- subset(toclean, eval(parse(text=condition)))
gc()
runData <- toclean

if (outtype == "Rdata") {
    save(runData, file=outfile)
} else {
    write.csv(runData, file=outfile, row.names=F)
}

warnings()
