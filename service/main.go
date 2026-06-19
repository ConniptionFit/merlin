package main

import (
	"log"
	"net/http"

	"github.com/merlin-assistant/service/handler"
)

func main() {
	mux := http.NewServeMux()
	mux.HandleFunc("/health", handler.HandleHealth)
	mux.HandleFunc("/query", handler.HandleQuery)
	mux.HandleFunc("/feedback", handler.HandleFeedback)

	addr := ":8080"
	log.Printf("Merlin service listening on %s", addr)
	log.Fatal(http.ListenAndServe(addr, withCORS(mux)))
}

func withCORS(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, X-Gemini-Key, X-Latitude, X-Longitude")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next.ServeHTTP(w, r)
	})
}
