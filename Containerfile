FROM emscripten/emsdk:3.1.74 AS build

WORKDIR /app

COPY src ./src
COPY data ./data
COPY Makefile.release.wasm .

RUN make -f Makefile.release.wasm

FROM nginx:alpine

COPY nginx.conf /etc/nginx/conf.d/default.conf

COPY --from=build /app/obj_wasm/main.js /usr/share/nginx/html/
COPY --from=build /app/obj_wasm/main.wasm /usr/share/nginx/html/
COPY --from=build /app/obj_wasm/main.data /usr/share/nginx/html/
COPY src_wasm/index.html /usr/share/nginx/html/index.html

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
