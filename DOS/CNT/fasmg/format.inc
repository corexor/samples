
macro format? clause
	match =MZ?, clause
		include 'mz.inc'
	else
		err 'invalid argument'
	end match
end macro
