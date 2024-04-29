## Read a single event
readi1byte <- function(){
    readBin(to.read, integer(), size=1)
}
readi2byte <- function(){
    readBin(to.read, integer(), size=2)
}
readword <- function(n=1){
    readBin(to.read, integer(), size=2, n, signed=FALSE)
}
readdword <- function(n=1){
    readBin(to.read, integer(), size=4, n)
}
readi4byte <- function(){
    readBin(to.read, integer(), size=4)
}
readc <- function(){
    here <- seek(to.read,where=NA)
    c <- readBin(to.read, character())
    seek(to.read,where=here)
    readBin(to.read, integer(), size=4)
    c
}
