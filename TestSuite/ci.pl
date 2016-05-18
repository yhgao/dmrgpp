#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long qw(:config no_ignore_case);
use Ci;

my ($min,$max,$submit,$valgrind,$workdir,$restart,$n,$postprocess);
GetOptions(
'm=f' => \$min,
'M=f' => \$max,
'S' => \$submit, 
'valgrind=s' => \$valgrind,
'w=s' => \$workdir,
'r' => \$restart,
'P' => \$postprocess,
'n=f' => \$n);
defined($submit) or $submit = 0;
defined($valgrind) or $valgrind = "";
defined($workdir) or $workdir = "tests";
defined($restart) or $restart = 0;
defined($postprocess) or $postprocess = 0;

if (defined($n)) {
	if (defined($min) or defined($max)) {
		die "$0: -n cannot be used with either -m or -M\n";
	}

	$min = $n;
	$max = $n;
}

my $templateBatch = "batchDollarized.pbs";
my @tests;
Ci::getTests(\@tests);

prepareDir();

my $total = scalar(@tests);
for (my $i = 0; $i < $total; ++$i) {
	my $n = $tests[$i];
	next if (defined($min) and $n < $min);
	next if (defined($max) and $n > $max);
	
	my $ir = isRestart("../inputs/input$n.inp",$n);
	if ($restart) {
		next if (!$ir);
	} else {
		next if ($ir);
	}

	my $what = getPostProcess("../inputs/input$n.inp",$n);
	print STDERR "$0: Run $n has postprocess $what\n";
	if ($what ne "" and $postprocess) {
		postTest($n,$what,$submit);
		next;
	}

	procTest($n,$valgrind,$submit);
}

sub postTest
{
	my ($n,$what,$submit) = @_;
	# what == #ci observe arguments=something
	my @temp = split/\s/,$what;
	scalar(@temp) > 2 or die "$0: Not enough info in $what\n";
	$temp[0] eq "#ci" or die "$0: postprocess string does not start with #ci\n";
	$temp[1] eq "observe" or die "$0: can only postprocess observe for now, not $temp[1]\n";
	my $args;
	if ($temp[2] =~ /^arguments=(.+$)/) {
		$args = $1;
	}

	defined($args) or die "$0: observe must have arguments\n";

	my $cmd = "./observe -f ../inputs/input$n.inp $args";
	print STDERR "$0: postTest $cmd\n";
	my $batch = createBatch($n,$cmd);
	submitBatch($batch) if ($submit);
}

sub getPostProcess
{
	my ($file,$n) = @_;
	open(FILE, "$file") or return "";
	my $what;
	while (<FILE>) {
		if (/^\#ci /) {
			chomp;
			$what=$_;
		}
	}

	close(FILE);
	return "" if (!defined($what));
	return $what;
}

sub isRestart
{
	my ($file,$n) = @_;
	open(FILE, "$file") or return 0;
	my $so;
	while (<FILE>) {
		chomp;
		if (/SolverOptions=(.*$)/) {
			$so = $1;
			last;
		}
	}

	close(FILE);
	defined($so) or return 0;
	if ($so =~/restart/) { return 1; }
	return 0;
}

sub procTest
{
	my ($n,$tool,$submit) = @_;
	my $valgrind = ($tool eq "") ? "" : "valgrind --tool=$tool ";
	$valgrind .= " --callgrind-out-file=callgrind$n.out " if ($tool eq "callgrind");
	my $cmd = "$valgrind./dmrg -f ../inputs/input$n.inp &> output$n.txt";
	my $batch = createBatch($n,$cmd);
	submitBatch($batch) if ($submit);
}

sub createBatch
{
	my ($ind,$cmd) = @_;
	my $file = "Batch$ind.pbs";
	open(FOUT,">$file") or die "$0: Cannot write to $file: $!\n";

	open(FILE,"../$templateBatch") or die "$0: Cannot open ../$templateBatch: $!\n";

	while (<FILE>) {
		while (/\$\$([a-zA-Z0-9\[\]]+)/) {
			my $line = $_;
			my $name = $1;
			my $str = "\$".$name;
			my $val = eval "$str";
			defined($val) or die "$0: Undefined substitution for $name\n";
			$line =~ s/\$\$$name/$val/;
			$_ = $line;
		}

		print FOUT;
	}

	close(FILE);
	close(FOUT);

	print STDERR "$0: $file written\n";
	return $file;
}

sub submitBatch
{
	my ($batch,$extra) = @_;
	defined($extra) or $extra = "";
	sleep(1);
	print STDERR "$0: Submitted $batch $extra $batch\n";

	my $ret = `qsub $extra $batch`;
	chomp($ret);
	return $ret;
}

sub prepareDir
{
	my $b = (-r "$workdir");
	system("mkdir $workdir") if (!$b);
	my $cmd = "diff ../src/dmrg $workdir/dmrg &> /dev/null";
	my $ret = system($cmd);
	system("cp -a ../src/dmrg $workdir/") if ($ret != 0);
	$cmd = "diff ../src/observe $workdir/observe &> /dev/null";
	$ret = system($cmd);
	system("cp -a ../src/observe $workdir/") if ($ret != 0);
	chdir("$workdir/");
}
