#!/usr/bin/perl
use strict;
use warnings;

# *TODO: specify test count here
use Test::More qw(no_plan);

use Crypt::CBC;
use MIME::Base64;

my $init_vector = "\x00" x 8;
# my $key = pack("H*", "ef5a8376eb0c99fe0dafa487d15bec19cae63d1e25fe31d8d92f7ab0398246d70ee733108e47360e16359654571cf5bab6c3375b42cee4fa");
# my $key = "d263eb8a78034e40";
         #"8d082918aa369174";
my $key = "\x00" x 16;

my $cipher = Crypt::CBC->new( { cipher => 'Blowfish',
								regenerate_key => 0,
							  	key => $key,
							  	iv => $init_vector,
							  	header => 'none',
								prepend_iv => 0,
							  	keysize => 16 } );

#my $blocks = $cipher->blocksize();
#print "blocksize $blocks\n";

my $len;
my $input = "01234567890123456789012345678901234\n";
#my $input = "a whale of a tale I tell you lad, a whale of a tale for me, a whale of a tale and the fiddlers three";
$len = length($input);
is ($len, 36, "input length");

$len = length($key);
is ($len, 16, "key length");


my $encrypted = $cipher->encrypt($input);
is (length($encrypted), 40, "encrypted length");

open(FH, "blowfish.1.bin");
my $bin = scalar <FH>;
is ($encrypted, $bin, "matches openssl");
close(FH);

my $base64 = encode_base64($encrypted);
is ($base64, "LkGExDOMTNxFIGBg8gP43UvbQLz7xztNWwYF2kLrtwT4hD7LykOXJw==\n",
	"base64 output");

my $unbase64 = decode_base64($base64);
is( $encrypted, $unbase64, "reverse base64" );

my $output = $cipher->decrypt($unbase64);
is ($input, $output, "reverse encrypt");

$key = pack("H[32]", "526a1e07a19dbaed84c4ff08a488d15e");
$cipher = Crypt::CBC->new( { cipher => 'Blowfish',
								regenerate_key => 0,
							  	key => $key,
							  	iv => $init_vector,
							  	header => 'none',
								prepend_iv => 0,
							  	keysize => 16 } );
$encrypted = $cipher->encrypt($input);
is (length($encrypted), 40, "uuid encrypted length");
$output = $cipher->decrypt($encrypted);
is ($input, $output, "uuid reverse encrypt");

open(FH, "blowfish.2.bin");
$bin = scalar <FH>;
close(FH);
is( $encrypted, $bin, "uuid matches openssl" );

print encode_base64($encrypted);
