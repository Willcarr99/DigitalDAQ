######################################################################
##
## ReadMIDAS.R
##
## Read Midas data in R and plot it
######################################################################

datadir <- "~/midas/online/data/v1730/"
file <- "run00012.mid"
file <- paste(datadir,file,sep="")
to.read = file(file,"rb")
seek(to.read,where=0)

source("readfunctions.R")

## First skip past the odb
while(readLines(to.read,n=1) != "</odb>"){}
readi1byte()
##readLines(to.read,n=1)

## where are we (so we can get back in a hurry)
beginning <- seek(to.read,where=NA)

read.evtHeader <- function(){
    evtID <- readi2byte()
    evtMask <- readi2byte()
    evtSerial <- readdword()
    evtTime <- readdword()
    evtSize <- readdword()

#    cat("------------------------------------\n")
#    cat("EVENT",evtSerial,"\n")
#    cat("Type         :",evtID,"\n")
#    cat("Mask         :",evtMask,"\n")
#    cat("Time         :",evtTime,"\n")
#    cat("Size (bytes) :",evtSize,"\n")
#    cat("\n")

    evtSize  ## return the size
}

read.allbankHeader <- function(){

    allbankSize <- readdword()
    allbankFlags <- readdword()

##    cat("--> All Banks\n")
##    cat("    Total Size  :",allbankSize,"\n")
##    cat("\n")

    allbankSize  ## return the size
}

read.bankHeader <- function(bankName){

    bankName <- readc()
    bankType <- readdword()
    bankSize <- readdword()
##    readi2byte()

##    cat("----> Bank",bankName,"\n")
##    cat("      Type :",bankType,"\n")
##    cat("      Size :",bankSize,"\n")

    return(list(size=bankSize,name=bankName)) ## return the size including header
}

read.Event <- function(){
    
    ## the event header
    evtSize <- read.evtHeader()
    ## All banks
    banksSize <- read.allbankHeader()
    leftSize <- banksSize
    
    ## Read a bank
##    for(i in 1:32){
    data <- list(ADC1=numeric(), V1730=numeric())
    repeat{
        bank <- read.bankHeader()
        if(substr(bank$name,1,4) == "ADC1"){
##            array <- sapply(1:(bank$size/2),function(i)readword())
            data$ADC1 <- rbind(data$ADC1,readword(n=bank$size/2))
##            print(array)
        }
        if(substr(bank$name,1,4) == "1730"){
##            array <- sapply(1:(bank$size/2),function(i)readword())
            data$V1730 <- rbind(data$V1730,readword(n=bank$size/2))
##            print(array)
        }
        leftSize <- leftSize - bank$size - 12
##        cat("After reading this bank, there is",leftSize,"bytes left\n")
        if(leftSize <= 0 || length(leftSize)==0)break
    }

##    print(data)
    
    return(data)
    
}


## read an event
##ADC <- numeric()
##wave <- numeric()
##repeat{
##    data <- read.Event()
##    ADC <- c(ADC,data$ADC1[,1])
##    wave <- c(wave,data$V1730)
##}

## This is nice if you know the length
##waves <- lapply(1:20,function(i)read.Event()$V1730[1,])

## This may be slower because you need to compute the length every time...
waves <- list()
adc <- list()
repeat{
    result <- try(read.Event(),silent=TRUE)
#    print(result)
    if(inherits(result,"try-error"))break
    if(length(result$ADC1)==0 & length(result$V1730)==0)next
    waves[[length(waves)+1]] <- result$V1730[1,]
    adc[[length(adc)+1]] <- result$ADC1[1,]
}
waves <- waves[1:(length(waves)-1)]
adc <- adc[1:length(adc)-1]
## Make a 2D array of waves (columns are channels)
waves <- lapply(waves,function(x)matrix(x,ncol=4))
adc <- matrix(unlist(adc),ncol=32,byrow=TRUE)


## plot the events
while(dev.cur()>1)dev.off()
myX11()
#mypdf("4Chan.pdf")

plotevents <- 10
ptest <- waves[2:plotevents]

## Get the times on the x-axis (500 MHz)
xlen <- dim(ptest[[1]])[1]
x <- (1/500)*(1:xlen)

## x- and y-limits
ylim <- range(ptest)
ylim <- c(14400,16000)
xlim <- range(x) #c(250,300)
xlim=c(0,10)

#readline("Press <return to continue")
oldpar <- par(mfrow=c(2,2))
for(chn in 1:4){
    plot(range(x),y=ylim,type="n",xlab="time (us)",ylab="Intensity",
         xaxs="i",yaxs="i",main=paste("Chn",chn),xlim=xlim)
    ## the the individual waveforms one by one. Make them translucent so
    ## that it's more like an ossiloscope display
    pout <- lapply(ptest,function(y)lines(x,y=y[,chn],col=add.alpha("black",0.1)))
}

## Also draw the ADC histogram
myX11()
hist(adc[,1],breaks=2^12,xlab="Channel",ylab="Counts")



## seek to beginning
seek(to.read,where=beginning)

#dev.off()
