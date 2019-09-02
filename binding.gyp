{
  "targets": [
    {
      "target_name": "posix",
      "include_dirs": ["<!(node -e \"require('nan')\")"],
      "sources": [
	      "src/posix.cc"
      ]
    }
  ]
}
