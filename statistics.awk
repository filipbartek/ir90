# We will compute the mean and standard deviation of the numbers on stdin.
# We can interpret the standard deviation [*] like this:
#
# 	With probability 68%, the actual mean time M is within one S.D. s
#	around the measured mean m: M \in (m-s,m+s)
#
# [*] We assume the numbers (time taken) are approximately normally
# distributed around the measured mean.

BEGIN {
	sum = 0
	sqsum = 0
}

{
	sum = sum + $1
	sqsum = sqsum + $1*$1
}

END {
	n = NR
	mean = sum / n
	# Exercise: This method of variance computation might be
	# sub-optimal. Try to find out why and implement a better
	# method.
	var = (n * sqsum - sum * sum) / (n * (n-1))
	sd = sqrt(var)
	print mean, mean - sd, mean + sd, sd
}
